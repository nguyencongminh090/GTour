#include "TournamentManager.h"
#include "position.h"
#include <iostream>
#include <cassert>
#include <cmath>

// Compression preference for binary samples
static const LZ4F_preferences_t LZ4Pref = {.frameInfo        = {},
                                            .compressionLevel = 3,
                                            .autoFlush        = 0,
                                            .favorDecSpeed    = 0,
                                            .reserved         = {}};

TournamentManager::TournamentManager()
    : openings(nullptr), jq(nullptr), pgnSeqWriter(nullptr), sgfSeqWriter(nullptr), msgSeqWriter(nullptr), sampleFile(nullptr), initialized(false), running(false)
{
}

TournamentManager::~TournamentManager()
{
    stop();
}

void TournamentManager::init(const Options &opts, const std::vector<EngineOptions> &engOpts)
{
    if (initialized) stop();

    options = opts;
    eo = engOpts;

    jq = new JobQueue((int)eo.size(), options.rounds, options.games, options.gauntlet);
    openings = new Openings(options.openings.c_str(), options.random, options.srand);

    if (!options.pgn.empty())
        pgnSeqWriter = new SeqWriter(options.pgn.c_str(), "a" FOPEN_TEXT);

    if (!options.sgf.empty())
        sgfSeqWriter = new SeqWriter(options.sgf.c_str(), "a" FOPEN_TEXT);

    if (!options.msg.empty())
        msgSeqWriter = new SeqWriter(options.msg.c_str(), "a" FOPEN_TEXT);

    if (!options.sp.fileName.empty()) {
        if (options.sp.compress) {
            DIE_IF(0,
                   !(sampleFile = fopen(options.sp.fileName.c_str(), "w" FOPEN_BINARY)));
            // Init LZ4 context and write file headers
            DIE_IF(0,
                   LZ4F_isError(
                       LZ4F_createCompressionContext(&sampleFileLz4Ctx, LZ4F_VERSION)));
            char   buf[LZ4F_HEADER_SIZE_MAX];
            size_t headerSize =
                LZ4F_compressBegin(sampleFileLz4Ctx, buf, sizeof(buf), &LZ4Pref);
            fwrite(buf, sizeof(char), headerSize, sampleFile);
        }
        else if (options.sp.format != SAMPLE_FORMAT_CSV) {
            DIE_IF(0,
                   !(sampleFile = fopen(options.sp.fileName.c_str(), "a" FOPEN_BINARY)));
        }
        else {
            DIE_IF(0, !(sampleFile = fopen(options.sp.fileName.c_str(), "a" FOPEN_TEXT)));
        }
    }

    // Prepare Workers[]
    for (int i = 0; i < options.concurrency; i++) {
        std::string logName;

        if (options.log) {
            logName = format("c-gomoku-cli.%i.log", i + 1);
        }

        workers.push_back(new Worker(i, logName.c_str()));
    }
    
    initialized = true;
}

void TournamentManager::start()
{
    if (!initialized) return;
    if (running) return;

    for (int i = 0; i < options.concurrency; i++) {
        threads.emplace_back(&TournamentManager::thread_start, this, workers[i]);
    }
    running = true;
}

void TournamentManager::stop()
{
    // Join threads
    for (std::thread &th : threads) {
        if (th.joinable()) th.join();
    }
    threads.clear();

    for (Worker *worker : workers)
        delete worker;
    workers.clear();

    close_sample_file(false);

    if (pgnSeqWriter) { delete pgnSeqWriter; pgnSeqWriter = nullptr; }
    if (sgfSeqWriter) { delete sgfSeqWriter; sgfSeqWriter = nullptr; }
    if (msgSeqWriter) { delete msgSeqWriter; msgSeqWriter = nullptr; }

    if (openings) { delete openings; openings = nullptr; }
    if (jq) { delete jq; jq = nullptr; }
    
    initialized = false;
    running = false;
}

void TournamentManager::close_sample_file(bool signal_exit)
{
    if (sampleFile) {
        std::aligned_storage_t<sizeof(FileLock)> lock_storage;

        if (signal_exit) {
            // We only do contructor of FileLock here...
            new (&lock_storage) FileLock(sampleFile);
        }

        if (options.sp.compress) {
            // Flush LZ4 tails and release LZ4 context
            const size_t bufSize = LZ4F_compressBound(0, nullptr);
            char         buf[bufSize];
            size_t       size = LZ4F_compressEnd(sampleFileLz4Ctx, buf, bufSize, nullptr);
            fwrite(buf, 1, size, sampleFile);
            LZ4F_freeCompressionContext(sampleFileLz4Ctx);
        }

        fclose(sampleFile);
        sampleFile = nullptr;
    }
}

bool TournamentManager::update()
{
    if (!running || !jq) return false;

    // Check deadlines
    for (int i = 0; i < options.concurrency; i++) {
        int64_t overdue = workers[i]->deadline_overdue();
        if (overdue > 0) {
            workers[i]->deadline_callback_once();
        }
        else if (overdue > 3000) {
            if (workers[i]->log)
                fprintf(workers[i]->log,
                        "deadline: %s is unresponsive [%s] after %" PRId64 "\n",
                        workers[i]->deadline.engineName.c_str(),
                        workers[i]->deadline.description.c_str(),
                        workers[i]->deadline.timeLimit);
            
             // For library usage, strictly DIE() might not be ideal (aborts whole app), 
             // but keeping original behavior for now.
            DIE("[%d] engine %s is unresponsive to [%s]\n",
                workers[i]->id,
                workers[i]->deadline.engineName.c_str(),
                workers[i]->deadline.description.c_str());
        }
    }

    if (jq->done()) {
        return false; // Tournament done
    }
    
    return true; // Still running
}

TournamentProgress TournamentManager::getProgress() const
{
    TournamentProgress p;
    if (jq) {
        std::lock_guard lock(jq->mtx);
        p.gamesCompleted = jq->completed;
        p.gamesTotal     = jq->jobs.size();
    }
    {
        std::lock_guard lock(progressMtx);
        p.lastResult = lastResult;
        p.board = currentBoard;
        // Drain log buffer (move lines out)
    p.logLines = std::move(const_cast<std::vector<std::string>&>(logBuffer));
        const_cast<std::vector<std::string>&>(logBuffer).clear();
    } // unlock progressMtx

    // Populate worker statuses
    int64_t now = system_msec();
    for (auto* w : workers) {
        std::lock_guard<std::mutex> lock(w->deadline.mtx);
        if (w->deadline.set) {
             WorkerStatus ws;
             ws.workerId = w->id;
             {
                 std::lock_guard<std::mutex> plock(progressMtx);
                 if (workerGameIndices.count(w->id)) ws.gameIdx = workerGameIndices.at(w->id);
                 else ws.gameIdx = 0;
             }
             ws.engineName = w->deadline.engineName;
             ws.timeLeft = w->deadline.timeLimit - now;
             if (ws.timeLeft < 0) ws.timeLeft = 0;
             p.workerStatuses.push_back(ws);
        }
    }

    p.pairResults = buildPairResults();
    p.isRunning = running;
    return p;
}

void TournamentManager::setLastResult(const std::string &result)
{
    std::lock_guard lock(progressMtx);
    lastResult = result;
}

void TournamentManager::addLog(const std::string &line)
{
    std::lock_guard lock(progressMtx);
    if (logBuffer.size() < 500)
        logBuffer.push_back(line);
}

std::vector<PairResult> TournamentManager::buildPairResults() const
{
    std::vector<PairResult> out;
    if (!jq) return out;

    std::lock_guard lock(jq->mtx);
    for (size_t i = 0; i < jq->results.size(); i++) {
        const auto& r = jq->results[i];
        if (r.total() == 0) continue;
        PairResult pr;
        pr.name1  = jq->names[r.ei[0]].empty() ? ("Engine" + std::to_string(r.ei[0]+1)) : jq->names[r.ei[0]];
        pr.name2  = jq->names[r.ei[1]].empty() ? ("Engine" + std::to_string(r.ei[1]+1)) : jq->names[r.ei[1]];
        pr.wins   = r.count[RESULT_WIN];
        pr.losses = r.count[RESULT_LOSS];
        pr.draws  = r.count[RESULT_DRAW];
        pr.total  = r.total();
        pr.score  = (pr.wins + 0.5 * pr.draws) / pr.total;
        out.push_back(pr);
    }
    return out;
}

void TournamentManager::updateBoardSnapshot(const Position& pos)
{
    std::lock_guard lock(progressMtx);
    int size = pos.get_size();
    currentBoard.resize(size);
    
    // Read the move history to reconstruct the board
    int moveCount = pos.get_move_count();
    const move_t* moves = pos.get_hist_moves();
    
    for (int i = 0; i < moveCount; i++) {
        Pos p = PosFromMove(moves[i]);
        Color c = ColorFromMove(moves[i]);
        int x = CoordX(p);
        int y = size - 1 - CoordY(p);
        
        BoardState::Cell cell = (c == BLACK) ? BoardState::BLACK : BoardState::WHITE;
        currentBoard.set(x, y, cell);
        currentBoard.moveOrder.push_back({x, y});
        
        // Track last move
        currentBoard.lastMoveX = x;
        currentBoard.lastMoveY = y;
    }
}

void TournamentManager::thread_start(Worker *w)
{
    std::string opening_str, messages;
    size_t      idx = 0, count = 0;     // game idx and count (shared across workers)
    Job         job   = {};
    Engine engines[2] = {{w, options.debug, !options.msg.empty() ? &messages : nullptr},
                         {w, options.debug, !options.msg.empty() ? &messages : nullptr}};
    
    // Set up message forwarding to GUI log and eval handling
    for (int i = 0; i < 2; i++) {
        Engine* eng = &engines[i];
        eng->onMessage = [this, eng](const std::string& msg) {
            addLog(eng->name + ": " + msg);
        };
        eng->onInfo = [this, eng, &job, &idx, &engines](const Info& info, int ply) {
            // Calculate winrate
            double winrate = 0.5;
            if (abs(info.score) > 20000) { // Mate
                winrate = (info.score > 0) ? 1.0 : 0.0;
            } else {
                // Sigmoid: 1 / (1 + e^(-x/200))
                winrate = 1.0 / (1.0 + std::exp(-info.score / 200.0));
            }
            
            if (this->onEngineEval) {
                EvalInfo ei;
                ei.gameIdx = idx;
                ei.moveIdx = ply;
                // Identify engine index (0 or 1) in the current game context
                ei.engineIdx = (eng == &engines[0]) ? 0 : 1;
                ei.score = info.score;
                ei.isMate = abs(info.score) > 20000;
                ei.winrate = winrate;
                this->onEngineEval(ei);
            }
        };
    }

    int    ei[2]      = {-1, -1};  // eo[ei[0]] plays eo[ei[1]]: initialize with invalid
                                   // values to start

    while (jq->pop(job, idx, count)) {
        {
            std::lock_guard<std::mutex> lock(progressMtx);
            workerGameIndices[w->id] = idx + 1; // 1-based index for display
        }

        // Clear all previous engine messages and write game index
        if (!options.msg.empty()) {
            messages = "----------------------------------------\n";
            messages += format("Game ID: %zu\n", idx + 1);
        }

        // Engine stop/start, as needed
        for (int i = 0; i < 2; i++) {
            if (job.ei[i] != ei[i]) {
                ei[i] = job.ei[i];
                engines[i].terminate();
                engines[i].start(eo[ei[i]].cmd.c_str(),
                                 eo[ei[i]].name.c_str(),
                                 eo[ei[i]].tolerance);
                jq->set_name(ei[i], engines[i].name);
            }
            // Re-init engine if it crashed/timeout previously
            else if (!engines[i].is_ok() || engines[i].is_crashed()) {
                engines[i].terminate();
                engines[i].start(eo[ei[i]].cmd.c_str(),
                                 eo[ei[i]].name.c_str(),
                                 eo[ei[i]].tolerance);
            }
        }

        // Choose opening position
        size_t openingRound =
            openings->next(opening_str, options.repeat ? idx / 2 : idx, w->id);

        // Play 1 game
        Game  game(job.round, job.game, w);
        Color color = BLACK;  // black play first in gomoku/renju by default

        if (!game.load_opening(opening_str, options, openingRound, color)) {
            DIE("[%d] illegal OPENING '%s'\n", w->id, opening_str.c_str());
        }

        // Set per-move callback to update the board snapshot for the GUI
        game.onMove = [this](const Position& p) {
            updateBoardSnapshot(p);
        };

        const int blackIdx = color ^ job.reverse;
        const int whiteIdx = oppositeColor((Color)blackIdx);

        {
            std::string msg = format("[%d] Started game %zu of %zu (%s vs %s)",
                w->id,
                idx + 1,
                count,
                engines[blackIdx].name.c_str(),
                engines[whiteIdx].name.c_str());
            printf("%s\n", msg.c_str());
            addLog(msg);
        }

        if (!options.msg.empty())
            messages += format("Engines: %s x %s\n",
                               engines[blackIdx].name,
                               engines[whiteIdx].name);

        const EngineOptions *eoPair[2] = {&eo[ei[0]], &eo[ei[1]]};
        const int            wld       = game.play(options, engines, eoPair, job.reverse);

        if (!options.gauntlet || !options.saveLoseOnly || wld == RESULT_LOSS) {
            // Write to PGN file
            if (pgnSeqWriter)
                pgnSeqWriter->push(idx, game.export_pgn(idx + 1));

            // Write to SGF file
            if (sgfSeqWriter)
                sgfSeqWriter->push(idx, game.export_sgf(idx + 1));

            // Write engine messages to TXT file
            if (msgSeqWriter)
                msgSeqWriter->push(idx, messages);

            // Write to Sample file
            if (sampleFile)
                game.export_samples(sampleFile, options.sp.format, sampleFileLz4Ctx);
        }

        // Write to stdout a one line summary of the game
        const char *ResultTxt[3] = {"0-1", "1/2-1/2", "1-0"};  // Black-White
        std::string result, reason;
        game.decode_state(result, reason, ResultTxt);

        {
            std::string summary = format("Game %zu: %s vs %s: %s {%s}",
                idx + 1,
                engines[blackIdx].name.c_str(),
                engines[whiteIdx].name.c_str(),
                result.c_str(),
                reason.c_str());
            setLastResult(summary);
            printf("[%d] %s\n", w->id, summary.c_str());
            addLog(summary);
        }

        // Pair update
        int wldCount[3] = {0};
        jq->add_result(job.pair, wld, wldCount);
        const int n =
            wldCount[RESULT_WIN] + wldCount[RESULT_LOSS] + wldCount[RESULT_DRAW];
        {
            std::string scoreMsg = format("Score of %s vs %s: %d - %d - %d  [%.3f] %d",
                   engines[0].name.c_str(),
                   engines[1].name.c_str(),
                   wldCount[RESULT_WIN],
                   wldCount[RESULT_LOSS],
                   wldCount[RESULT_DRAW],
                   (wldCount[RESULT_WIN] + 0.5 * wldCount[RESULT_DRAW]) / n,
                   n);
            printf("%s\n", scoreMsg.c_str());
            addLog(scoreMsg);
        }

        // SPRT update
        if (options.sprt && options.sprtParam.done(wldCount)) {
            jq->stop();
        }

        // Tournament update
        jq->print_results((size_t)options.games);
    }

    for (int i = 0; i < 2; i++) {
        engines[i].terminate();
    }
}

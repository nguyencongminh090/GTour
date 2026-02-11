#pragma once

#include "BoardState.h"
#include "engine.h"
#include "extern/lz4frame.h"
#include "game.h"
#include "jobs.h"
#include "openings.h"
#include "options.h"
#include "seqwriter.h"
#include "sprt.h"
#include "util.h"
#include "workers.h"

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <map>

// Per-pair result snapshot for GUI
struct PairResult {
    std::string name1, name2;
    int wins = 0, losses = 0, draws = 0;
    double score = 0.0;
    int total = 0;
};

// Thread-safe progress info snapshot
struct WorkerStatus {
    int workerId;
    size_t gameIdx;
    std::string engineName; // The engine currently thinking
    std::string blackName;
    std::string whiteName;
    bool isBlackActive;
    int64_t timeLeft; // milliseconds (for backward compatibility/active player)
    int64_t blackTime;
    int64_t whiteTime;
};

// Thread-safe progress info snapshot
struct TournamentProgress {
    size_t gamesCompleted = 0;
    size_t gamesTotal     = 0;
    std::string lastResult;
    bool   isRunning      = false;
    BoardState  board;           // snapshot of the current game board
    std::vector<std::string> logLines;   // new log lines since last poll
    std::vector<PairResult>  pairResults; // current standings
    std::vector<WorkerStatus> workerStatuses; // status of active workers
};

class TournamentManager {
public:
    TournamentManager();
    ~TournamentManager();

    // Initialize the tournament with settings
    void init(const Options &opts, const std::vector<EngineOptions> &engOpts);

    // Start the worker threads
    void start();

    // internal check for deadlines and updates (call this periodically, e.g. every 100ms)
    // Returns true if tournament is still running, false if done.
    bool update();

    // Stop and clean up
    void stop();

    // Thread-safe progress snapshot for the GUI
    TournamentProgress getProgress() const;

    struct EvalInfo {
        size_t gameIdx;
        int    moveIdx;
        int    engineIdx; // 0 or 1
        int    score;     // centipawns or mate distance
        bool   isMate;
        double winrate;
    };

    std::function<void(const EvalInfo&)> onEngineEval;

private:
    void thread_start(Worker *w);
    void close_sample_file(bool signal_exit);
    void updateBoardSnapshot(const Position& pos);
    void setLastResult(const std::string &result);
    void addLog(const std::string &line);
    std::vector<PairResult> buildPairResults() const;

    Options                    options;
    std::vector<EngineOptions> eo;
    Openings                  *openings;
    JobQueue                  *jq;
    SeqWriter                 *pgnSeqWriter;
    SeqWriter                 *sgfSeqWriter;
    SeqWriter                 *msgSeqWriter;
    std::vector<Worker *>      workers;
    std::vector<std::thread>   threads;
    
    FILE                      *sampleFile;
    LZ4F_compressionContext_t  sampleFileLz4Ctx;
    
    bool                       initialized;
    bool                       running;

    // Progress tracking (thread-safe)
    mutable std::mutex         progressMtx;
    std::string                lastResult;
    BoardState                 currentBoard;
    std::vector<std::string>   logBuffer;    // pending log lines for GUI
    struct ActiveGameInfo {
        size_t idx;
        std::string blackName;
        std::string whiteName;
        int64_t blackTime = 0;
        int64_t whiteTime = 0;
    };
    std::map<int, ActiveGameInfo> workerGameInfos; // active game info per worker
};

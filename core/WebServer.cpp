#include "WebServer.h"
#include "extern/httplib.h"

#include <sstream>
#include <iomanip>
#include <filesystem>

// ─── JSON helpers ────────────────────────────────────────────────────────────

static std::string escapeJSON(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c;
        }
    }
    return out;
}

// ─── WebServer ───────────────────────────────────────────────────────────────

WebServer::WebServer(TournamentManager& mgr, int p)
    : manager(mgr), svr(new httplib::Server()), port(p)
{
    // Try to find the web/ directory relative to the executable
    // Look in several common locations
    std::vector<std::string> candidates = {
        "web",
        "../web",
        "./web",
        "src/web",
    };
    
    for (auto& path : candidates) {
        if (std::filesystem::is_directory(path)) {
            webRoot = std::filesystem::absolute(path).string();
            break;
        }
    }
}

WebServer::~WebServer() {
    stop();
    delete svr;
}

void WebServer::start() {
    if (running.load()) return;

    setupRoutes();
    running.store(true);

    serverThread = std::thread([this]() {
        printf("[WebServer] Listening on http://localhost:%d\n", port);
        svr->listen("0.0.0.0", port);
        running.store(false);
    });
}

void WebServer::stop() {
    running.store(false);  // signal SSE loops to exit first
    if (svr) svr->stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

bool WebServer::isRunning() const {
    return running.load();
}

// ─── JSON Serialization ─────────────────────────────────────────────────────

std::string WebServer::boardToJSON(const BoardState& b) const {
    std::ostringstream ss;
    ss << "{";
    ss << "\"size\":" << b.boardSize << ",";

    // cells as flat array
    ss << "\"cells\":[";
    for (int i = 0; i < (int)b.cells.size(); i++) {
        if (i > 0) ss << ",";
        ss << (int)b.cells[i];
    }
    ss << "],";

    ss << "\"lastMoveX\":" << b.lastMoveX << ",";
    ss << "\"lastMoveY\":" << b.lastMoveY << ",";

    // move order
    ss << "\"moveOrder\":[";
    for (int i = 0; i < (int)b.moveOrder.size(); i++) {
        if (i > 0) ss << ",";
        ss << "[" << b.moveOrder[i].first << "," << b.moveOrder[i].second << "]";
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

std::string WebServer::progressToJSON(const TournamentProgress& p) const {
    std::ostringstream ss;
    ss << "{";
    ss << "\"gamesCompleted\":" << p.gamesCompleted << ",";
    ss << "\"gamesTotal\":" << p.gamesTotal << ",";
    ss << "\"lastResult\":\"" << escapeJSON(p.lastResult) << "\",";

    // Board
    ss << "\"board\":" << boardToJSON(p.board) << ",";

    // Workers
    ss << "\"workers\":[";
    for (int i = 0; i < (int)p.workerStatuses.size(); i++) {
        if (i > 0) ss << ",";
        auto& w = p.workerStatuses[i];
        ss << "{";
        ss << "\"id\":" << w.workerId << ",";
        ss << "\"gameIdx\":" << w.gameIdx << ",";
        ss << "\"engineName\":\"" << escapeJSON(w.engineName) << "\",";
        ss << "\"blackName\":\"" << escapeJSON(w.blackName) << "\",";
        ss << "\"whiteName\":\"" << escapeJSON(w.whiteName) << "\",";
        ss << "\"isBlackActive\":" << (w.isBlackActive ? "true" : "false") << ",";
        ss << "\"blackTime\":" << w.blackTime << ",";
        ss << "\"whiteTime\":" << w.whiteTime;
        ss << "}";
    }
    ss << "],";

    // Pair results
    ss << "\"results\":[";
    for (int i = 0; i < (int)p.pairResults.size(); i++) {
        if (i > 0) ss << ",";
        auto& r = p.pairResults[i];
        ss << "{";
        ss << "\"name1\":\"" << escapeJSON(r.name1) << "\",";
        ss << "\"name2\":\"" << escapeJSON(r.name2) << "\",";
        ss << "\"wins\":" << r.wins << ",";
        ss << "\"losses\":" << r.losses << ",";
        ss << "\"draws\":" << r.draws << ",";
        ss << "\"score\":" << std::fixed << std::setprecision(3) << r.score << ",";
        ss << "\"total\":" << r.total;
        ss << "}";
    }
    ss << "],";

    // Log lines
    ss << "\"logLines\":[";
    for (int i = 0; i < (int)p.logLines.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\"" << escapeJSON(p.logLines[i]) << "\"";
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

// ─── Route Setup ─────────────────────────────────────────────────────────────

void WebServer::setupRoutes() {
    // CORS + cache-busting headers for all responses
    svr->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"},
        {"Cache-Control", "no-store, no-cache, must-revalidate"},
        {"Pragma", "no-cache"}
    });

    // REST: Full state snapshot
    svr->Get("/api/state", [this](const httplib::Request&, httplib::Response& res) {
        try {
            auto progress = manager.getProgress();
            res.set_content(progressToJSON(progress), "application/json");
        } catch (const std::exception& e) {
            printf("[WebServer] /api/state error: %s\n", e.what());
            res.status = 500;
            res.set_content("{\"error\":\"internal\"}", "application/json");
        }
    });

    // REST: Board only
    svr->Get("/api/board", [this](const httplib::Request&, httplib::Response& res) {
        try {
            auto progress = manager.getProgress();
            res.set_content(boardToJSON(progress.board), "application/json");
        } catch (const std::exception& e) {
            printf("[WebServer] /api/board error: %s\n", e.what());
            res.status = 500;
            res.set_content("{\"error\":\"internal\"}", "application/json");
        }
    });

    // Health check / ping endpoint
    svr->Get("/api/ping", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // Serve static files from web/ directory
    if (!webRoot.empty()) {
        svr->set_mount_point("/", webRoot);
        printf("[WebServer] Serving static files from: %s\n", webRoot.c_str());
    } else {
        // Fallback: serve a minimal inline page
        svr->Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(
                "<!DOCTYPE html><html><body>"
                "<h1>Gomoku Tournament Live</h1>"
                "<p>Web files not found. Place index.html in the web/ directory.</p>"
                "<p>API available at <a href='/api/state'>/api/state</a></p>"
                "</body></html>",
                "text/html"
            );
        });
    }
}

#pragma once

#include "TournamentManager.h"
#include <atomic>
#include <thread>
#include <string>

// Forward declare to avoid pulling in httplib.h in the header
namespace httplib { class Server; }

// Embeddable HTTP server that exposes tournament state as a REST API + SSE stream.
// Viewers open http://localhost:<port> in a browser to watch the live tournament.
class WebServer {
public:
    WebServer(TournamentManager& mgr, int port = 8080);
    ~WebServer();

    void start();             // launches server in a background thread
    void stop();              // stops the server and joins the thread
    bool isRunning() const;
    int  getPort() const { return port; }

private:
    void setupRoutes();
    std::string progressToJSON(const TournamentProgress& p) const;
    std::string boardToJSON(const BoardState& b) const;

    TournamentManager& manager;
    httplib::Server*    svr;
    std::thread         serverThread;
    int                 port;
    std::atomic<bool>   running{false};
    std::string         webRoot;  // path to web/ static files
};

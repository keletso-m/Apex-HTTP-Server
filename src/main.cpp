#include "tcp_server.h"
#include "http_parser.h"
#include "static_handler.h"
#include "router.h"
#include "logger.h"
#include "config.h"        
#include <iostream>
#include <csignal>
#include <cstring>
#include "rate_limiter.h"
#include "metrics.h"
#include <sstream>

static TCPServer* g_server = nullptr;

  
void handle_signal(int sig) {
    std::cout << "\n[INFO] Caught signal " << sig << ", shutting down...\n";
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    // prevent SIGPIPE from crashing the server when writing to a closed socket
    signal(SIGPIPE, SIG_IGN);
    // Accept optional config path as argument, default to config/server.conf
    std::string config_path = (argc > 1) ? argv[1] : "config/server.conf";

    // Load config first (logger not ready yet, Config warns to stderr)
    ServerConfig cfg = Config::load(config_path);
    RateLimiter limiter(cfg.rate_limit_per_second); 
    Metrics metrics;

    // Init logger with values from config
    Logger::instance().init(cfg.log_file, LogLevel::DEBUG);
    LOG_INFO("=== Apex HTTP Server — Phase 3 ===");
    LOG_INFO("Loaded config from: " + config_path);

    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    StaticFileHandler file_handler(cfg.document_root);
    

    // router 
    Router router;
    // Health check endpoint, for Docker/load balancers
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });
    // static files as prefix catch all
    router.get_prefix("/", [&](const HttpRequest& req) {
    return file_handler.handle(req);
    });
    // 405 for POST/PUT/DELETE on any path
    router.set_fallback([](const HttpRequest& req) {
        return HttpParser::make_error(404, "Not found: " + req.path);
    });
    // metrics endpoint
    router.get("/metrics", [&](const HttpRequest&) {
    std::ostringstream out;
    out << "# HELP apex_requests_total Total HTTP requests received\n";
    out << "# TYPE apex_requests_total counter\n";
    out << "apex_requests_total " << metrics.total_requests() << "\n";
    out << "# HELP apex_uptime_seconds Server uptime in seconds\n";
    out << "# TYPE apex_uptime_seconds gauge\n";
    out << "apex_uptime_seconds " << metrics.uptime_seconds() << "\n";
    out << "# HELP apex_active_connections Current tracked connections\n";
    out << "# TYPE apex_active_connections gauge\n";
    out << "apex_active_connections " << (g_server ? g_server->active_connections() : 0) << "\n";
    return HttpParser::make_response(200, out.str(), "text/plain; version=0.0.4");
});

 auto handler = [&](int /*client_fd*/, const std::string& raw) -> HandlerResult {
        if (!limiter.allow()) {
            HttpResponse res = HttpParser::make_error(429, "Rate limit exceeded");
            res.keep_alive = false;   // safest default when request isnt parsed yet
            return { res.serialize(), false };
        }

        if (raw.empty())
            return { HttpParser::make_error(400, "Empty request").serialize(), false };

        HttpRequest req = HttpParser::parse(raw);
        if (!req.valid)
            return { HttpParser::make_error(400, "Malformed HTTP request").serialize(), false };

        LOG_INFO(req.method + " " + req.path);

        HttpResponse res = router.route(req);
        return { res.serialize(), res.keep_alive };
    };

    try {
       TCPServer server(cfg.host, cfg.port, cfg.backlog, cfg.threads, cfg.keep_alive_timeout);
        g_server = &server;

        std::cout << "  Config             : " << config_path << "\n";
        std::cout << "  Serving files from : " << cfg.document_root << "\n";
        std::cout << "  Listening on       : http://" << cfg.host << ":" << cfg.port << "\n";
        std::cout << "  Worker threads     : " << cfg.threads << "\n";
        std::cout << "  Press Ctrl+C to stop.\n\n";

        server.run(handler);
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal: " + std::string(e.what()));
        return 1;
    }

    LOG_INFO("Server stopped.");
    return 0;
}
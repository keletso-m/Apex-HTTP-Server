#include "tcp_server.h"
#include "http_parser.h"
#include "static_handler.h"
#include "logger.h"
#include "config.h"
#include <iostream>
#include <csignal>
#include <cstring>

static TCPServer* g_server = nullptr;

void handle_signal(int sig) {
    std::cout << "\n[INFO] Caught signal " << sig << ", shutting down...\n";
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    // Accept optional config path as argument, default to config/server.conf
    std::string config_path = (argc > 1) ? argv[1] : "config/server.conf";

    // Load config first (logger not ready yet, Config warns to stderr)
    ServerConfig cfg = Config::load(config_path);

    // Init logger with values from config
    Logger::instance().init(cfg.log_file, LogLevel::DEBUG);
    LOG_INFO("=== Apex HTTP Server — Phase 3 ===");
    LOG_INFO("Loaded config from: " + config_path);

    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    StaticFileHandler file_handler(cfg.document_root);

    auto handler = [&](int /*client_fd*/, const std::string& raw) -> std::string {
        if (raw.empty())
            return HttpParser::make_error(400, "Empty request").serialize();

        HttpRequest req = HttpParser::parse(raw);
        if (!req.valid)
            return HttpParser::make_error(400, "Malformed HTTP request").serialize();

        LOG_INFO(req.method + " " + req.path);

        if (req.method != "GET" && req.method != "HEAD")
            return HttpParser::make_error(405, "Method not allowed").serialize();

        return file_handler.handle(req).serialize();
    };

    try {
        TCPServer server(cfg.host, cfg.port, cfg.backlog, cfg.threads);
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
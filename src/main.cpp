#include "tcp_server.h"
#include "http_parser.h"
#include "static_handler.h"
#include "logger.h"

#include <iostream>
#include <csignal>
#include <cstring>

// ─── Config (Phase 1: hardcoded, Phase 3 will add config file) ────────────────
static const std::string HOST          = "0.0.0.0";
static const int         PORT          = 8080;
static const std::string DOCUMENT_ROOT = "./www";
static const std::string LOG_FILE      = "./logs/server.log";

// ─── Signal handling ──────────────────────────────────────────────────────────
static TCPServer* g_server = nullptr;

void handle_signal(int sig) {
    std::cout << "\n[INFO] Caught signal " << sig << ", shutting down...\n";
    if (g_server) g_server->stop();
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    // Init logger
    Logger::instance().init(LOG_FILE, LogLevel::DEBUG);
    LOG_INFO("=== Apex HTTP Server — Phase 1 ===");

    // Signal handlers
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    // Static file handler
    StaticFileHandler file_handler(DOCUMENT_ROOT);

    // Request handler lambda — this is the "router" for Phase 1
    auto handler = [&](int /*client_fd*/, const std::string& raw) -> std::string {
        if (raw.empty())
            return HttpParser::make_error(400, "Empty request").serialize();

        HttpRequest req = HttpParser::parse(raw);
        if (!req.valid)
            return HttpParser::make_error(400, "Malformed HTTP request").serialize();

        LOG_INFO(req.method + " " + req.path);

        // Only serve GET/HEAD for now
        if (req.method != "GET" && req.method != "HEAD")
            return HttpParser::make_error(405, "Method not allowed").serialize();

        return file_handler.handle(req).serialize();
    };

    try {
        TCPServer server(HOST, PORT, /*backlog=*/10);
        g_server = &server;

        std::cout << "  Serving files from : " << DOCUMENT_ROOT << "\n";
        std::cout << "  Listening on       : http://" << HOST << ":" << PORT << "\n";
        std::cout << "  Press Ctrl+C to stop.\n\n";

        server.run(handler);
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal: " + std::string(e.what()));
        return 1;
    }

    LOG_INFO("Server stopped.");
    return 0;
}

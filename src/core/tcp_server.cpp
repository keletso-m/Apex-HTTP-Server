#include "tcp_server.h"
#include "logger.h"

#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

TCPServer::TCPServer(const std::string& host, int port, int /*backlog*/)
    : server_fd_(-1), port_(port), host_(host), running_(false)
{
    setup_socket();
    LOG_INFO("TCPServer created on " + host + ":" + std::to_string(port));
}

TCPServer::~TCPServer() {
    if (server_fd_ >= 0) close(server_fd_);
}

void TCPServer::setup_socket() {
    // Create socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));

    // Allow port reuse (avoids "Address already in use" on restart)
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port_);
    addr.sin_addr.s_addr = (host_ == "0.0.0.0") ? INADDR_ANY
                           : inet_addr(host_.c_str());

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));

    // Listen
    if (listen(server_fd_, 10) < 0)
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
}

void TCPServer::run(RequestHandler handler) {
    running_ = true;
    LOG_INFO("Server listening on port " + std::to_string(port_));

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) LOG_WARN("accept() failed: " + std::string(strerror(errno)));
            continue;
        }

        // Log connection
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        LOG_INFO("New connection from " + std::string(ip));

        // Read raw request (simple, no timeout for Phase 1)
        char buf[4096] = {};
        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        std::string raw_request(buf, n > 0 ? n : 0);

        // Call handler and send response
        std::string response = handler(client_fd, raw_request);
        send(client_fd, response.c_str(), response.size(), 0);

        close(client_fd);
    }
}

void TCPServer::stop() {
    running_ = false;
    close(server_fd_);
    server_fd_ = -1;
}

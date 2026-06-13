#include "tcp_server.h"
#include "logger.h"

#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h> 

static const int MAX_EVENTS = 64;

TCPServer::TCPServer(const std::string& host, int port, int backlog, size_t threads)
    : server_fd_(-1), port_(port), host_(host),
      running_(false), backlog_(backlog),
      pool_(threads)
{
    setup_socket();
    LOG_INFO("TCPServer created on " + host + ":" + std::to_string(port)
             + " (" + std::to_string(threads) + " worker threads)");
}

TCPServer::~TCPServer() {
    if (server_fd_ >= 0) close(server_fd_);
}

void TCPServer::setup_socket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port_);
    addr.sin_addr.s_addr = (host_ == "0.0.0.0") ? INADDR_ANY
                           : inet_addr(host_.c_str());

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));

    if (listen(server_fd_, backlog_) < 0)   // backlog_ used now
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
}

void TCPServer::run(RequestHandler handler) {
    running_ = true;
    // create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        throw std::runtime_error("epoll_create1() failed: " + std::string(strerror(errno)));
    // register server socket with epoll
    //EPOLLIN for read events, EPOLLET for edge-triggered mode
    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd_, &ev) < 0)
        throw std::runtime_error("epoll_ctl() failed: " + std::string(strerror(errno)));
    // 
    // Give the pool a closure that does recv → handler → send → close
    pool_.set_handler([handler](WorkItem item) {
        char buf[4096] = {};
        ssize_t n = recv(item.client_fd, buf, sizeof(buf) - 1, 0);
        std::string raw(buf, n > 0 ? n : 0);

        std::string response = handler(item.client_fd, raw);
        send(item.client_fd, response.c_str(), response.size(), 0);
        close(item.client_fd);
    });

    LOG_INFO("Server listening on port " + std::to_string(port_) + " (epoll)");
    epoll_event events[MAX_EVENTS];

    // event loop
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) LOG_WARN("accept() failed: " + std::string(strerror(errno)));
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        LOG_INFO("New connection from " + std::string(ip));

        WorkItem item{ client_fd, std::string(ip) };

        if (!pool_.enqueue(item)) {
            // Queue full — send 503 and drop
            const char* busy = "HTTP/1.1 503 Service Unavailable\r\n"
                               "Content-Length: 0\r\n"
                               "Connection: close\r\n\r\n";
            send(client_fd, busy, strlen(busy), 0);
            close(client_fd);
        }
    }
}

void TCPServer::stop() {
    running_ = false;
    pool_.stop();           // drains queue first
    close(server_fd_);
    server_fd_ = -1;
}
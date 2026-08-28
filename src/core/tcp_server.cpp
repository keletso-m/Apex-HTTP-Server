#include "tcp_server.h"
#include "logger.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <memory>

static const int MAX_EVENTS = 64;

struct ConnectionInfo {
    std::string ip;
    std::chrono::steady_clock::time_point last_activity;
};

TCPServer::TCPServer(const std::string& host, int port, int backlog,
                     size_t threads, int keep_alive_timeout_seconds)
    : server_fd_(-1), port_(port), host_(host),
      running_(false), backlog_(backlog),
      pool_(threads),
      keep_alive_timeout_seconds_(keep_alive_timeout_seconds)
{
    setup_socket();
    LOG_INFO("TCPServer created on " + host + ":" + std::to_string(port)
             + " (" + std::to_string(threads) + " worker threads, "
             + std::to_string(keep_alive_timeout_seconds_) + "s keep-alive timeout)");
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

    if (listen(server_fd_, backlog_) < 0)
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
}

void TCPServer::run(RequestHandler handler) {
    running_ = true;

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        throw std::runtime_error("epoll_create1() failed: " + std::string(strerror(errno)));

    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd_, &ev) < 0)
        throw std::runtime_error("epoll_ctl() failed: " + std::string(strerror(errno)));
    connections_ = std::make_shared<std::unordered_map<int, ConnectionInfo>>();
    conn_mutex_   = std::make_shared<std::mutex>();
    int timeout_secs = keep_alive_timeout_seconds_;
    // fd -> connection info (ip + last activity time), shared across the
    // event loop and worker threads. One map replaces the old client_ips-only one.
    auto connections = std::make_shared<std::unordered_map<int, ConnectionInfo>>();
    auto conn_mutex   = std::make_shared<std::mutex>();
    int  timeout_secs = keep_alive_timeout_seconds_;

    // Worker: recv -> handler -> send -> either re-arm for keep-alive or close.
    pool_.set_handler([handler, epoll_fd = epoll_fd_, connections = connections_, conn_mutex = conn_mutex_](WorkItem item) {
        char buf[4096] = {};
        ssize_t n = recv(item.client_fd, buf, sizeof(buf) - 1, 0);

        auto close_conn = [&]() {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, item.client_fd, nullptr); // ignore error
            close(item.client_fd);
            std::lock_guard<std::mutex> lock(*conn_mutex);
            connections->erase(item.client_fd);
        };

        if (n <= 0) {
            // n == 0: client closed. n < 0: recv error.
            close_conn();
            return;
        }

        std::string raw(buf, n);
        HandlerResult result = handler(item.client_fd, raw);

        ssize_t sent = send(item.client_fd, result.data.c_str(), result.data.size(), 0);
        if (sent < 0) {
            close_conn();
            return;
        }

        if (!result.keep_alive) {
            close_conn();
            return;
        }

        // Keep-alive: refresh activity timestamp, then re-arm the fd in epoll
        // so the next request wakes a (possibly different) worker thread.
        {
            std::lock_guard<std::mutex> lock(*conn_mutex);
            auto it = connections->find(item.client_fd);
            if (it != connections->end())
                it->second.last_activity = std::chrono::steady_clock::now();
        }

        epoll_event cev{};
        cev.events  = EPOLLIN | EPOLLONESHOT;
        cev.data.fd = item.client_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, item.client_fd, &cev) < 0) {
            if (errno == ENOENT) {
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, item.client_fd, &cev) < 0) {
                    LOG_WARN("epoll_ctl(ADD) failed re-arming fd: " + std::string(strerror(errno)));
                    close_conn();
                }
            } else {
                LOG_WARN("epoll_ctl(MOD) failed re-arming fd: " + std::string(strerror(errno)));
                close_conn();
            }
        }
    });

    LOG_INFO("Server listening on port " + std::to_string(port_) + " (epoll)");
    epoll_event events[MAX_EVENTS];

    while (running_) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 500);

        if (nfds < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("epoll_wait() failed: " + std::string(strerror(errno)));
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd_) {
                // New connection.
                sockaddr_in client_addr{};
                socklen_t   client_len = sizeof(client_addr);

                int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    LOG_WARN("accept() failed: " + std::string(strerror(errno)));
                    continue;
                }

                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
                LOG_INFO("New connection from " + std::string(ip));

                {
                    std::lock_guard<std::mutex> lock(*conn_mutex);
                    (*connections)[client_fd] = ConnectionInfo{
                        std::string(ip), std::chrono::steady_clock::now()
                    };
                }

                WorkItem item{ client_fd, std::string(ip) };

                if (!pool_.enqueue(item)) {
                    const char* busy = "HTTP/1.1 503 Service Unavailable\r\n"
                                       "Content-Length: 0\r\n"
                                       "Connection: close\r\n\r\n";
                    send(client_fd, busy, strlen(busy), 0);
                    close(client_fd);
                    std::lock_guard<std::mutex> lock(*conn_mutex);
                    connections->erase(client_fd);
                }
            } else {
                // Existing keep-alive connection has more data.
                int fd = events[i].data.fd;
                std::string ip;
                {
                    std::lock_guard<std::mutex> lock(*conn_mutex);
                    auto it = connections->find(fd);
                    ip = (it != connections->end()) ? it->second.ip : "";
                }

                WorkItem item{ fd, ip };
                if (!pool_.enqueue(item)) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    std::lock_guard<std::mutex> lock(*conn_mutex);
                    connections->erase(fd);
                }
            }
        }

        // Idle sweep: close any connection that's exceeded the keep-alive timeout.
        auto now = std::chrono::steady_clock::now();
        std::vector<int> expired;
        {
            std::lock_guard<std::mutex> lock(*conn_mutex);
            for (auto& [fd, info] : *connections) {
                auto idle = std::chrono::duration_cast<std::chrono::seconds>(
                    now - info.last_activity).count();
                if (idle >= timeout_secs) {
                    expired.push_back(fd);
                }
            }
        }
        for (int fd : expired) {
            LOG_INFO("Closing idle keep-alive connection, fd=" + std::to_string(fd));
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            std::lock_guard<std::mutex> lock(*conn_mutex);
            connections->erase(fd);
        }
    }

    close(epoll_fd_);
    epoll_fd_ = -1;
}

void TCPServer::stop() {
    running_ = false;
    if (connections_ && conn_mutex_) {
        std::lock_guard<std::mutex> lock(*conn_mutex_);
        for (auto& [fd, info] : *connections_) {
            LOG_INFO("Shutdown: closing keep-alive connection fd=" + std::to_string(fd));
            if (epoll_fd_ >= 0)
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
        }
        connections_->clear();
    }
    pool_.stop();  // drains queue first 
    close(server_fd_);
    server_fd_ = -1;
}
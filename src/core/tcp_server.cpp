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
    // fd to ip, needed when a conncetion is redisacpached from epoll
    auto client_ips = std::make_shared<std::unordered_map<int, std::string>>();
    auto ip_mutex    = std::make_shared<std::mutex>();
    //Worker: recv -> handler -> send -> either re-arm for keep-alive or close
     pool_.set_handler([handler, epoll_fd, client_ips, ip_mutex](WorkItem item) {
        char buf[4096] = {};
        ssize_t n = recv(item.client_fd, buf, sizeof(buf) - 1, 0);

        auto close_conn = [&]() {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, item.client_fd, nullptr); // ignore error
            close(item.client_fd);
            std::lock_guard<std::mutex> lock(*ip_mutex);
            client_ips->erase(item.client_fd);
        };
        if (n <= 0) {
            // n == 0 client closed. n < 0 recv error
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
        // keep alive re-arm the fd in epoll so the next request wakes
        // maybe a different thread instead of locking this one 
        epoll_event cev{};
        cev.events  = EPOLLIN | EPOLLONESHOT;
        cev.data.fd = item.client_fd;
        // MOD if already registered in the 2nd + connection 
        // add if this is the time its handed to epoll
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

    // event loop
    while (running_) {
        // Block until up to MAX_EVENTS are ready, timeout = 500ms
        // The timeout lets us check running_ periodically for clean shutdown
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 500);

        if (nfds < 0) {
            if (errno == EINTR) continue;   // interrupted by signal, loop again
            LOG_ERROR("epoll_wait() failed: " + std::string(strerror(errno)));
            break;
        }

        // Handle each ready event 
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd_) continue; // for keep alive, we only accept new connections on the server_fd_

            // New connection ready to accept
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
                std::lock_guard<std::mutex> lock(*ip_mutex);
                (*client_ips)[client_fd] = ip;
            }

            WorkItem item{ client_fd, std::string(ip) };

            if (!pool_.enqueue(item)) {
                const char* busy = "HTTP/1.1 503 Service Unavailable\r\n"
                                   "Content-Length: 0\r\n"
                                   "Connection: close\r\n\r\n";
                send(client_fd, busy, strlen(busy), 0);
                close(client_fd);
                std::lock_guard<std::mutex> lock(*ip_mutex);
                client_ips->erase(client_fd);
            }
        }
    }

    close(epoll_fd);
}

void TCPServer::stop() {
    running_ = false;
    pool_.stop();           // drains queue first
    close(server_fd_);
    server_fd_ = -1;
}
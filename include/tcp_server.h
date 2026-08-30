#pragma once
#include "thread_pool.h"
#include <string>
#include <functional>
#include <netinet/in.h>
#include <mutex>
#include <chrono>
#include <memory>


struct HandlerResult {
    std::string data;
    bool keep_alive = false;
};

using RequestHandler = std::function<HandlerResult(int, const std::string&)>;

struct ConnectionInfo {
    std::string ip;
    std::chrono::steady_clock::time_point last_activity;
};

class TCPServer {
public:
    TCPServer(const std::string& host, int port, int backlog = 10,
        size_t threads = 4, int keep_alive_timeout_seconds = 60);  
    ~TCPServer();

    void run(RequestHandler handler);
    void stop();
    size_t active_connections() const;

private:
    int         server_fd_;
    int         port_;
    std::string host_;
    bool        running_;
    int         backlog_;      
    ThreadPool  pool_;
    int         keep_alive_timeout_seconds_;
    int epoll_fd_ = -1;
    std::shared_ptr<std::unordered_map<int, ConnectionInfo>> connections_;
    std::shared_ptr<std::mutex> conn_mutex_;

    void setup_socket();
    

};
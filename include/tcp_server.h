#pragma once

#include "thread_pool.h"
#include <string>
#include <functional>
#include <netinet/in.h>

static const int KEEP_ALIVE_TIMEOUT_SECONDS = 30;
struct HandlerResult {
    std::string data;
    bool keep_alive = false;
};

using RequestHandler = std::function<HandlerResult(int, const std::string&)>;

class TCPServer {
public:
    TCPServer(const std::string& host, int port, int backlog = 10,
          size_t threads = 4, int keep_alive_timeout_seconds = 60);
    ~TCPServer();

    void run(RequestHandler handler);
    void stop();

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
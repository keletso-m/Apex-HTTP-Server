#pragma once

#include "thread_pool.h"
#include <string>
#include <functional>
#include <netinet/in.h>

using RequestHandler = std::function<std::string(int client_fd, const std::string& raw_request)>;

struct HandlerResult {
    std::string data;
    bool keep_alive = false;
};

using RequestHandler = std::function<HandlerResult(int, const std::string&)>;

class TCPServer {
public:
    TCPServer(const std::string& host, int port, int backlog = 10, size_t threads = 4);
    ~TCPServer();

    void run(RequestHandler handler);
    void stop();

private:
    int         server_fd_;
    int         port_;
    std::string host_;
    bool        running_;
    int         backlog_;       // now actually used
    ThreadPool  pool_;

    void setup_socket();
    

};
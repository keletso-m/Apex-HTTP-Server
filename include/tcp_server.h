#pragma once

#include <string>
#include <functional>
#include <netinet/in.h>

// Callback type: receives client fd, returns response string
using RequestHandler = std::function<std::string(int client_fd, const std::string& raw_request)>;

class TCPServer {
public:
    TCPServer(const std::string& host, int port, int backlog = 10);
    ~TCPServer();

    // Block and accept connections, calling handler for each
    void run(RequestHandler handler);
    void stop();

private:
    int         server_fd_;
    int         port_;
    std::string host_;
    bool        running_;

    void setup_socket();
};

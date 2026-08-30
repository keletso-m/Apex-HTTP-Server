#include <gtest/gtest.h>
#include "tcp_server.h"
#include "http_parser.h"
#include "router.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace {

class TestClient {
public:
    bool connect_to(int port) {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        return connect(fd_, (sockaddr*)&addr, sizeof(addr)) == 0;
    }

    std::string send_and_receive(const std::string& request) {
        send(fd_, request.c_str(), request.size(), 0);

        char buf[8192] = {};
        ssize_t n = recv(fd_, buf, sizeof(buf) - 1, 0);
        return std::string(buf, n > 0 ? n : 0);
    }

    void close_conn() {
        if (fd_ >= 0) { close(fd_); fd_ = -1; }
    }

    ~TestClient() { close_conn(); }

private:
    int fd_ = -1;
};

Router build_test_router() {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });
    router.set_fallback([](const HttpRequest& req) {
        return HttpParser::make_error(404, "Not found: " + req.path);
    });
    return router;
}
}
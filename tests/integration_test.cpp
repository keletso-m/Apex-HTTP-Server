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
// namespace 
class IntegrationTest : public ::testing::Test {
protected:
    static constexpr int TEST_PORT = 18080;

    void SetUp() override {
        router_ = build_test_router();

        auto handler = [this](int /*fd*/, const std::string& raw) -> HandlerResult {
            if (raw.empty())
                return { HttpParser::make_error(400, "Empty request").serialize(), false };

            HttpRequest req = HttpParser::parse(raw);
            if (!req.valid)
                return { HttpParser::make_error(400, "Malformed HTTP request").serialize(), false };

            HttpResponse res = router_.route(req);
            return { res.serialize(), res.keep_alive };
        };

        server_ = std::make_unique<TCPServer>("127.0.0.1", TEST_PORT, 10, 2, 5);

        server_thread_ = std::thread([this, handler]() {
            server_->run(handler);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void TearDown() override {
        server_->stop();
        if (server_thread_.joinable()) server_thread_.join();
        server_.reset();
    }

    Router router_;
    std::unique_ptr<TCPServer> server_;
    std::thread server_thread_;
};
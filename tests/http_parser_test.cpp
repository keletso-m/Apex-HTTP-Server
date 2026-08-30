#include <gtest/gtest.h>
#include "http_parser.h"

// requestline parsing tests
TEST(HttpParserTest, ParsesBasicGetRequest) {
    std::string raw = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);

    EXPECT_TRUE(req.valid);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_EQ(req.version, "HTTP/1.1");
}

TEST(HttpParserTest, RejectsEmptyRequest) {
    HttpRequest req = HttpParser::parse("");
    EXPECT_FALSE(req.valid);
}

TEST(HttpParserTest, RejectsRequestWithNoHeaderTerminator) {
    // No \r\n\r\n 
    HttpRequest req = HttpParser::parse("GET / HTTP/1.1\r\nHost: localhost\r\n");
    EXPECT_FALSE(req.valid);
}

// keep alive and overrides 
TEST(HttpParserTest, Http11DefaultsToKeepAlive) {
    std::string raw = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);
    EXPECT_TRUE(req.keep_alive);
}

TEST(HttpParserTest, Http10DefaultsToClose) {
    std::string raw = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);
    EXPECT_FALSE(req.keep_alive);
}

TEST(HttpParserTest, ExplicitCloseHeaderOverridesHttp11Default) {
    std::string raw = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);
    EXPECT_FALSE(req.keep_alive);
}

TEST(HttpParserTest, ExplicitKeepAliveHeaderOverridesHttp10Default) {
    std::string raw = "GET / HTTP/1.0\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);
    EXPECT_TRUE(req.keep_alive);
}

// header key normalization 
TEST(HttpParserTest, HeaderKeysAreLowercased) {
    std::string raw = "GET / HTTP/1.1\r\nHOST: localhost\r\nX-Custom-Header: value\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);

    EXPECT_TRUE(req.headers.count("host"));
    EXPECT_TRUE(req.headers.count("x-custom-header"));
    EXPECT_FALSE(req.headers.count("HOST"));
}

TEST(HttpParserTest, LowercaseConnectionHeaderIsRecognized) {
    // Client sends lowercase 
    std::string raw = "GET / HTTP/1.1\r\nconnection: close\r\n\r\n";
    HttpRequest req = HttpParser::parse(raw);
    EXPECT_FALSE(req.keep_alive);
}

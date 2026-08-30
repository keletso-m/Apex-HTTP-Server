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
    // No \r\n\r\n at all — incomplete/malformed
    HttpRequest req = HttpParser::parse("GET / HTTP/1.1\r\nHost: localhost\r\n");
    EXPECT_FALSE(req.valid);
}

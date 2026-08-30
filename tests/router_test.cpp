#include <gtest/gtest.h>
#include "router.h"
#include "http_parser.h"

namespace {

HttpRequest make_request(const std::string& method, const std::string& path, bool keep_alive = true) {
    HttpRequest req;
    req.method = method;
    req.path = path;
    req.version = "HTTP/1.1";
    req.valid = true;
    req.keep_alive = keep_alive;
    return req;
}

}
// exact vs prefix matching 
TEST(RouterTest, MatchesExactPath) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });

    HttpResponse res = router.route(make_request("GET", "/health"));
    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.body, "OK");
}
TEST(RouterTest, ExactRouteDoesNotMatchLongerPath) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });
    router.set_fallback([](const HttpRequest&) {
        return HttpParser::make_error(404, "Not found");
    });

    HttpResponse res = router.route(make_request("GET", "/health/extra"));
    EXPECT_EQ(res.status_code, 404);
}

TEST(RouterTest, MatchesPrefixRoute) {
    Router router;
    router.get_prefix("/static/", [](const HttpRequest& req) {
        return HttpParser::make_response(200, "file:" + req.path, "text/plain");
    });

    HttpResponse res = router.route(make_request("GET", "/static/css/main.css"));
    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.body, "file:/static/css/main.css");
}
TEST(RouterTest, FirstMatchingRouteWinsInRegistrationOrder) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "specific", "text/plain");
    });
    router.get_prefix("/", [](const HttpRequest&) {
        return HttpParser::make_response(200, "catch-all", "text/plain");
    });

    HttpResponse res = router.route(make_request("GET", "/health"));
    EXPECT_EQ(res.body, "specific");
}

// method handling
TEST(RouterTest, UnsupportedMethodReturns405) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });

    HttpResponse res = router.route(make_request("DELETE", "/health"));
    EXPECT_EQ(res.status_code, 405);
}

TEST(RouterTest, MethodMismatchOnMatchedPathReturns405) {
    Router router;
    router.post("/submit", [](const HttpRequest&) {
        return HttpParser::make_response(200, "submitted", "text/plain");
    });

    HttpResponse res = router.route(make_request("GET", "/submit"));
    EXPECT_EQ(res.status_code, 405);
}
TEST(RouterTest, HeadIsAllowedOnGetRoute) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });

    HttpResponse res = router.route(make_request("HEAD", "/health"));
    EXPECT_EQ(res.status_code, 200);
}

TEST(RouterTest, HeadRequestSetsSkipBody) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });

    HttpResponse res = router.route(make_request("HEAD", "/health"));
    EXPECT_TRUE(res.skip_body);
}
TEST(RouterTest, GetRequestDoesNotSetSkipBody) {
    Router router;
    router.get("/health", [](const HttpRequest&) {
        return HttpParser::make_response(200, "OK", "text/plain");
    });

    HttpResponse res = router.route(make_request("GET", "/health"));
    EXPECT_FALSE(res.skip_body);
}

// no match/ fallback 
TEST(RouterTest, NoMatchWithFallbackCallsFallback) {
    Router router;
    router.set_fallback([](const HttpRequest& req) {
        return HttpParser::make_error(404, "Not found: " + req.path);
    });

    HttpResponse res = router.route(make_request("GET", "/nonexistent"));
    EXPECT_EQ(res.status_code, 404);
}

TEST(RouterTest, NoMatchWithoutFallbackReturns404) {
    Router router;

    HttpResponse res = router.route(make_request("GET", "/anything"));
    EXPECT_EQ(res.status_code, 404);
}


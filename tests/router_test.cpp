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
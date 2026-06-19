#pragma once

#include "http_parser.h"
#include <string>
#include <vector>
#include <functional>

// A route handler takes a request and returns a response
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string method;      // "GET", "POST", "" meaning any method
    std::string path;        // exact "/health", prefix "/static/"
    bool        prefix;      // true = match any path starting with this
    RouteHandler handler;
};
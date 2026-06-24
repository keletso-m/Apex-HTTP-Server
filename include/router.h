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
class Router {
public:
    // Register exact path routes
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void any(const std::string& path, RouteHandler handler);
    // register prfix routes lke "/" cattches eevertyhing 
    void get_prefix(const std::string& path, RouteHandler handler);
    
    // set fallback, its calledif no route matches
    void set_fallback(RouteHandler handler);

    // route a request, and returns a serialized resppnse string 
    std::string route(const HttpRequest& request) const;

private:
    std::vector<Route> routes_;
    RouteHandler fallback_;
    void add_route(const std::string& method, const std::string& path, bool prefix, RouteHandler handler);

};
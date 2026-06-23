 #include "router.h"
 #include "http_parser.h"
 #include "logger.h"

 void Router::add_route(const std::string& method, const std::string& path,
                       bool prefix, RouteHandler handler) {
    routes_.push_back({ method, path, prefix, std::move(handler) });
}

void Router::get(const std::string& path, RouteHandler handler) {
    add_route("GET", path, false, std::move(handler));
}
void Router::post(const std::string& path, RouteHandler handler) {
    add_route("POST", path, false, std::move(handler));
}

void Router::any(const std::string& path, RouteHandler handler) {
    add_route("", path, false, std::move(handler));
}
void Router::get_prefix(const std::string& path, RouteHandler handler) {
    add_route("GET", path, true, std::move(handler));
}
void Router::set_fallback(RouteHandler handler) {
    fallback_ = std::move(handler);
}
std::string Router::route(const HttpRequest& req) const {
    // Check method, only GET/HEAD allowed for now
    if (req.method != "GET" && req.method != "HEAD" && req.method != "POST")
        return HttpParser::make_error(405, "Method Not Allowed").serialize();

     // Walk routes in registration order,first match wins
    for (const auto& r : routes_) {
        // Method check
        if (!r.method.empty() && r.method != req.method) {
            // HEAD is handled like GET
            if (!(r.method == "GET" && req.method == "HEAD"))
                continue;
        }

        // Path check
        bool matched = r.prefix
         ? req.path.rfind(r.path, 0) == 0 // starts with the prefix
         : req.path == r.path; // exact match

          if (matched) {
            LOG_INFO(req.method + " " + req.path + " → matched route " + r.path);
            return r.handler(req).serialize();
        }
    }
    // no route matched, call fallback if set
    if (fallback_) {
        LOG_INFO(req.method + " " + req.path + " → matched fallback route");
        return fallback_(req).serialize();
    }
    return HttpParser::make_error(404, "No route for " + req.path).serialize();
}

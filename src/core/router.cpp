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
// route a request, returns the response (not yet serialized)
HttpResponse Router::route(const HttpRequest& req) const {
    if (req.method != "GET" && req.method != "HEAD" && req.method != "POST") {
        HttpResponse res = HttpParser::make_error(405, "Method Not Allowed");
        res.keep_alive = req.keep_alive;
        return res;
    }

    for (const auto& r : routes_) {
        bool matched = r.prefix
            ? req.path.rfind(r.path, 0) == 0
            : req.path == r.path;

        if (!matched) continue;

        bool method_ok = r.method.empty()
            || r.method == req.method
            || (r.method == "GET" && req.method == "HEAD");
        if (!method_ok) {
            HttpResponse res = HttpParser::make_error(405, "Method Not Allowed");
            res.keep_alive = req.keep_alive;
            return res;
        }

        LOG_INFO(req.method + " " + req.path + " → matched route " + r.path);
        HttpResponse res = r.handler(req);
        if (req.method == "HEAD") {
            LOG_DEBUG("HEAD request — skipping body for: " + req.path);
            res.skip_body = true;
        }
        res.keep_alive = req.keep_alive;
        return res;
    }

    if (fallback_) {
        LOG_INFO(req.method + " " + req.path + " → matched fallback route");
        HttpResponse res = fallback_(req);
        res.keep_alive = req.keep_alive;
        return res;
    }

    HttpResponse res = HttpParser::make_error(404, "No route for " + req.path);
    res.keep_alive = req.keep_alive;
    return res;
}
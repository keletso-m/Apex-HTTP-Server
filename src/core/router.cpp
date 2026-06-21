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

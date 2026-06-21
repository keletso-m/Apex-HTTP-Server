 #include "router.h"
 #include "http_parser.h"
 #include "logger.h"

 void Router::add_route(const std::string& method, const std::string& path,
                       bool prefix, RouteHandler handler) {
    routes_.push_back({ method, path, prefix, std::move(handler) });
}

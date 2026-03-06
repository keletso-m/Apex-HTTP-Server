#pragma once

#include "http_parser.h"
#include <string>

class StaticFileHandler {
public:
    explicit StaticFileHandler(const std::string& document_root);

    // Returns an HttpResponse for the requested path
    HttpResponse handle(const HttpRequest& req) const;

private:
    std::string document_root_;

    std::string resolve_path(const std::string& uri_path) const;
    std::string get_content_type(const std::string& filepath) const;
    std::string read_file(const std::string& filepath) const;
};

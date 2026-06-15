#pragma once
#include "http_parser.h"
#include <string>
#include <mutex>
#include <unordered_map>

// Cached file entry
struct CachedFile {
    std::string content;
    std::string content_type;
};


class StaticFileHandler {
public:
    explicit StaticFileHandler(const std::string& document_root);

    // Returns an HttpResponse for the requested path
    HttpResponse handle(const HttpRequest& req) const;

private:
    std::string document_root_;
    // file cache populated on first read, never evicted 
    mutable std::unordered_map<std::string, CachedFile> cache_;
    mutable std::mutex cache_mutex_;
    static const size_t MAX_CACHE_FILE_SIZE = 1024 * 1024; // 1MB limit
    std::string resolve_path(const std::string& uri_path) const;
    std::string get_content_type(const std::string& filepath) const;
    std::string read_file(const std::string& filepath) const;
};

#include "static_handler.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>


namespace fs = std::filesystem;

StaticFileHandler::StaticFileHandler(const std::string& document_root)
    : document_root_(document_root) {}

HttpResponse StaticFileHandler::handle(const HttpRequest& req) const {
    if (req.method != "GET" && req.method != "HEAD")
        return HttpParser::make_error(405, "Method Not Allowed");

    std::string filepath = resolve_path(req.path);
    // ADD: resolve_path returns "" on path traversal attempt
    if (filepath.empty())
        return HttpParser::make_error(403, "Forbidden");
    LOG_DEBUG("Serving: " + filepath);
    
    // resolve directory -> index.html, check existence, read file, determine content type
    if (fs::exists(filepath) && fs::is_directory(filepath)) {
        // Try index.html inside the directory
        std::string index = filepath + "/index.html";
        if (fs::exists(index)) filepath = index;
        else return HttpParser::make_error(403, "Directory listing not allowed");
    }

     if (!fs::exists(filepath)) 
        return HttpParser::make_error(404, "File not found: " + req.path);
    // cache lookup
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(filepath);
        if (it != cache_.end()) {
            LOG_DEBUG("Cache hit: " + filepath);
            auto res = HttpParser::make_response(200, it->second.content,
                                                 it->second.content_type);
            return res;
        }
    }
    // cache miss, read file from disk
     LOG_DEBUG("Cache miss, reading: " + filepath);

     // skip caching files larger than 1MB
    std::error_code ec;
    auto filesize = fs::file_size(filepath, ec);
    if (ec) return HttpParser::make_error(500, "Could not stat file");

    std::string body = read_file(filepath);
    if (body.empty() && filesize > 0)
        return HttpParser::make_error(500, "Could not read file");

    std::string ct = get_content_type(filepath);

    // cache the file if it's smaller files
    if (filesize <= MAX_CACHE_FILE_SIZE) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_[filepath] = { body, ct };
        LOG_DEBUG("Cached: " + filepath + " (" + std::to_string(filesize) + " bytes)");
    }
    return HttpParser::make_response(200, body, ct);
}

std::string StaticFileHandler::resolve_path(const std::string& uri_path) const {
    // Strip query string
    std::string clean = uri_path;
    auto q = clean.find('?');
    if (q != std::string::npos) clean = clean.substr(0, q);

    // Basic path traversal guard
    if (clean.find("..") != std::string::npos)
        return "";

    return document_root_ + clean;
}

std::string StaticFileHandler::get_content_type(const std::string& filepath) const {
    auto ext = fs::path(filepath).extension().string();
    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".txt")  return "text/plain";
    return "application/octet-stream";
}

std::string StaticFileHandler::read_file(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

#include "static_handler.h"
#include "logger.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

StaticFileHandler::StaticFileHandler(const std::string& document_root)
    : document_root_(document_root) {}

HttpResponse StaticFileHandler::handle(const HttpRequest& req) const {
    if (req.method != "GET" && req.method != "HEAD")
        return HttpParser::make_error(405, "Method Not Allowed");

    std::string filepath = resolve_path(req.path);
    LOG_DEBUG("Serving: " + filepath);

    if (!fs::exists(filepath))
        return HttpParser::make_error(404, "File not found: " + req.path);

    if (fs::is_directory(filepath)) {
        // Try index.html inside the directory
        std::string index = filepath + "/index.html";
        if (fs::exists(index)) filepath = index;
        else return HttpParser::make_error(403, "Directory listing not allowed");
    }

    std::string body = read_file(filepath);
    if (body.empty() && !fs::is_empty(filepath))
        return HttpParser::make_error(500, "Could not read file");

    std::string ct = get_content_type(filepath);
    auto res = HttpParser::make_response(200, body, ct);
    res.status_text = "OK";
    return res;
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

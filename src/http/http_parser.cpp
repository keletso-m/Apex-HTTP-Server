#include "http_parser.h"
#include <sstream>
#include <stdexcept>

//shared status text map for responses, used by both make_response and make_error

static const std::unordered_map<int, std::string> STATUS_TEXTS = {
    {200, "OK"},
    {201, "Created"},
    {204, "No Content"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {500, "Internal Server Error"},
    {503, "Service Unavailable"},
};

//  Parse a raw HTTP request string into an HttpRequest struct

HttpRequest HttpParser::parse(const std::string& raw) {
    HttpRequest req;
    if (raw.empty()) return req;
    //find end of headers
    size_t header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) return req;  // incomplete request
    if (header_end > HttpLimits::MAX_HEADER_SECTION) {
        // Header section too large reject
        return req; // invalid; caller sends 400
    }

    std::string header_section = raw.substr(0, header_end);
    std::istringstream stream(header_section);
    std::string line;
    
    // Request line: METHOD PATH VERSION
    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream req_line(line);
    req_line >> req.method >> req.path >> req.version;
    if (req.method.empty() || req.path.empty()) return req;
    if (req.path.size() > HttpLimits::MAX_URI_LENGTH) return req; // reject oversized URI
    // default to keep-alive for HTTP/1.1, close for HTTP/1.0
    req.keep_alive = (req.version == "HTTP/1.1");


    // Headers: key: value
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue; // blank line = end of headers

        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key   = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim leading space from value
            if (!value.empty() && value[0] == ' ') value = value.substr(1);
            for (auto& c : key) c = std::tolower(c);   // normalize key casing
            req.headers[key] = value;
        }

    }
    // explicitly check for Connection header to override default keep-alive behavior
    auto it = req.headers.find("connection");
    if (it != req.headers.end()) {
        std::string val = it->second;
        for (auto& c : val) c = std::tolower(c);
        if (val.find("keep-alive") != std::string::npos) req.keep_alive = true;
        else if (val.find("close") != std::string::npos) req.keep_alive = false;
    }


    // Body, read content length bytes after header terminator 
    size_t body_start = header_end + 4; // skip "\r\n\r\n"
    auto cl_it = req.headers.find("content-length");
    if (cl_it != req.headers.end()) {
        size_t content_length = 0;
        try {
            content_length = std::stoul(cl_it->second);
        } catch (...) {
            return req; // invalid Content-Length, reject as malformed
        }

        size_t available = raw.size() - body_start;
        if (available < content_length) {
            // Body not fully received yet in this buffer. With my current
            // single-recv()-per-request model 
            // flag as invalid for now rather than silently truncating.
            return req;
        }

        req.body = raw.substr(body_start, content_length);
    }
    // no Content-Length header,no body

    req.valid = true;
    return req;
}

//  Serialize response to string for sending over socket

std::string HttpResponse::serialize() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    for (auto& [k, v] : headers)
        out << k << ": " << v << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n";
    out << "\r\n";
    if (!skip_body) // skip body for HEAD requests
        out << body;
    return out.str();
}

// Helpers to create responses and errors with standard status text


HttpResponse HttpParser::make_response(int code, const std::string& body,
                                       const std::string& content_type) {
    HttpResponse res;
    res.status_code = code;
    res.status_text = STATUS_TEXTS.count(code) ? STATUS_TEXTS.at(code) : "Unknown";
    res.headers["Content-Type"] = content_type;
    res.body = body;
    return res;
}


HttpResponse HttpParser::make_error(int code, const std::string& message) {
    HttpResponse res;
    res.status_code = code;
    res.status_text = STATUS_TEXTS.count(code) ? STATUS_TEXTS.at(code) : "Error";
    res.headers["Content-Type"] = "text/html";
    res.body = "<html><body><h1>" + std::to_string(code) + " " + res.status_text
             + "</h1><p>" + message + "</p></body></html>";
    return res;
}
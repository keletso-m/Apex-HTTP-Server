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

//  Parse 

HttpRequest HttpParser::parse(const std::string& raw) {
    HttpRequest req;
    if (raw.empty()) return req;

    std::istringstream stream(raw);
    std::string line;

    // Request line: METHOD PATH VERSION
    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream req_line(line);
    req_line >> req.method >> req.path >> req.version;
    if (req.method.empty() || req.path.empty()) return req;
    // default to keep-alive for HTTP/1.1, close for HTTP/1.0
    req.keep_alive = (req.version == "HTTP/1.1");

    // Headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break; // blank line = end of headers

        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key   = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim leading space from value
            if (!value.empty() && value[0] == ' ') value = value.substr(1);
            req.headers[key] = value;
        }

    }

    // Body
    std::string body_buf;
    while (std::getline(stream, line)) body_buf += line + "\n";
    req.body  = body_buf;
    req.valid = true;
    return req;
}

//  Serialize response 

std::string HttpResponse::serialize() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    for (auto& [k, v] : headers)
        out << k << ": " << v << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "\r\n";
    if (!skip_body) // skip body for HEAD requests
        out << body;
    return out.str();
}

// Helpers 


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
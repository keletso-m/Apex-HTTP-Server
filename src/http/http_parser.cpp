#include "http_parser.h"
#include <sstream>
#include <stdexcept>

// ─── Parse ────────────────────────────────────────────────────────────────────

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

    // Body (whatever remains)
    std::string body_buf;
    while (std::getline(stream, line)) body_buf += line + "\n";
    req.body  = body_buf;
    req.valid = true;
    return req;
}

// ─── Serialize response ───────────────────────────────────────────────────────

std::string HttpResponse::serialize() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    for (auto& [k, v] : headers)
        out << k << ": " << v << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n";
    out << "\r\n";
    out << body;
    return out.str();
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

HttpResponse HttpParser::make_response(int code, const std::string& body,
                                       const std::string& content_type) {
    HttpResponse res;
    res.status_code = code;
    res.status_text = (code == 200) ? "OK" : "Unknown";
    res.headers["Content-Type"] = content_type;
    res.body = body;
    return res;
}

HttpResponse HttpParser::make_error(int code, const std::string& message) {
    static const std::unordered_map<int, std::string> STATUS_TEXTS = {
        {400, "Bad Request"}, {403, "Forbidden"},
        {404, "Not Found"},   {500, "Internal Server Error"}
    };
    HttpResponse res;
    res.status_code = code;
    res.status_text = STATUS_TEXTS.count(code) ? STATUS_TEXTS.at(code) : "Error";
    res.headers["Content-Type"] = "text/html";
    res.body = "<html><body><h1>" + std::to_string(code) + " " + res.status_text
             + "</h1><p>" + message + "</p></body></html>";
    return res;
}

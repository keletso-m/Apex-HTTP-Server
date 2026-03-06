#pragma once

#include <string>
#include <unordered_map>

struct HttpRequest {
    std::string method;   // GET, POST, etc.
    std::string path;     // /index.html
    std::string version;  // HTTP/1.1
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool valid = false;
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const;
};

class HttpParser {
public:
    static HttpRequest  parse(const std::string& raw);
    static HttpResponse make_response(int code, const std::string& body,
                                      const std::string& content_type = "text/plain");
    static HttpResponse make_error(int code, const std::string& message);
};

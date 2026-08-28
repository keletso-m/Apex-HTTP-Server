#pragma once

#include <string>
#include <unordered_map>

namespace HttpLimits {
    constexpr size_t MAX_HEADER_SECTION = 8192;   // 8KB, matches common server defaults (nginx: 8k)
    constexpr size_t MAX_URI_LENGTH     = 2048;   // 2KB
    constexpr size_t MAX_BODY_SIZE      = 10 * 1024 * 1024; // 10MB
}

struct HttpRequest {
    std::string method;   
    std::string path;     // /index.html
    std::string version;  // HTTP/1.1
    std::unordered_map<std::string, std::string> headers; // keys stored lowercase
    std::string body;
    bool valid = false;
    bool keep_alive = true; 

};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool skip_body = false; // set true for head response
    bool keep_alive = true; 
    std::string serialize() const;
};

class HttpParser {
public:
    static HttpRequest  parse(const std::string& raw);
    static HttpResponse make_response(int code, const std::string& body,
                                      const std::string& content_type = "text/plain");
    static HttpResponse make_error(int code, const std::string& message);
};

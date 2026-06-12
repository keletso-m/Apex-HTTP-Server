#pragma once

#include <string>
#include <unordered_map>

struct ServerConfig {
    // server
    std::string host        = "0.0.0.0";
    int         port        = 8080;
    int         threads     = 4;
    int         backlog     = 10;
    int         max_connections = 1000;

    // paths
    std::string document_root = "./www";
    std::string log_file      = "./logs/server.log";

    // performance
    int keep_alive_timeout = 60;
    int request_timeout    = 30;
};

class Config {
public:
    // Loads config from file, falls back to defaults if file missing
    static ServerConfig load(const std::string& filepath);

private:
    static std::unordered_map<std::string, std::string>
        parse_file(const std::string& filepath);

    static int   to_int(const std::unordered_map<std::string, std::string>& m,
                        const std::string& key, int default_val);
    static std::string to_str(const std::unordered_map<std::string, std::string>& m,
                        const std::string& key, const std::string& default_val);
};
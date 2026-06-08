#include "config.h"
#include "logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>

ServerConfig Config::load(const std::string& filepath) {
    ServerConfig cfg;
    auto values = parse_file(filepath);

    if (values.empty()) {
        LOG_WARN("Config file not found or empty: " + filepath + " — using defaults");
        return cfg;
    }

    cfg.host            = to_str(values, "host",            cfg.host);
    cfg.port            = to_int(values, "port",            cfg.port);
    cfg.threads         = to_int(values, "threads",         cfg.threads);
    cfg.backlog         = to_int(values, "backlog",         cfg.backlog);
    cfg.max_connections = to_int(values, "max_connections", cfg.max_connections);
    cfg.document_root   = to_str(values, "document_root",  cfg.document_root);
    cfg.log_file        = to_str(values, "log_file",       cfg.log_file);
    cfg.keep_alive_timeout = to_int(values, "keep_alive_timeout", cfg.keep_alive_timeout);
    cfg.request_timeout    = to_int(values, "request_timeout",    cfg.request_timeout);

    LOG_INFO("Config loaded from " + filepath);
    return cfg;
}

std::unordered_map<std::string, std::string>
Config::parse_file(const std::string& filepath) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(filepath);
    if (!file) return values;   // empty map → caller uses defaults

    std::string line;
    while (std::getline(file, line)) {
        // Strip comments and whitespace
        auto comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);

        // Skip section headers and blank lines
        if (line.empty() || line[0] == '[') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Trim whitespace from both
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(key);
        trim(value);

        if (!key.empty()) values[key] = value;
    }
    return values;
}

int Config::to_int(const std::unordered_map<std::string, std::string>& m,
                   const std::string& key, int default_val) {
    auto it = m.find(key);
    if (it == m.end()) return default_val;
    try { return std::stoi(it->second); }
    catch (...) {
        LOG_WARN("Invalid int value for config key '" + key + "' — using default");
        return default_val;
    }
}

std::string Config::to_str(const std::unordered_map<std::string, std::string>& m,
                            const std::string& key, const std::string& default_val) {
    auto it = m.find(key);
    return (it != m.end()) ? it->second : default_val;
}
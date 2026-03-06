#pragma once

#include <string>
#include <fstream>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance();

    void init(const std::string& log_file = "", LogLevel min_level = LogLevel::INFO);
    void log(LogLevel level, const std::string& message);

    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info (const std::string& msg) { log(LogLevel::INFO,  msg); }
    void warn (const std::string& msg) { log(LogLevel::WARN,  msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }

private:
    Logger() = default;
    std::ofstream file_;
    LogLevel      min_level_ = LogLevel::INFO;

    std::string level_str(LogLevel l) const;
    std::string timestamp()           const;
};

// Convenience macros
#define LOG_INFO(msg)  Logger::instance().info(msg)
#define LOG_WARN(msg)  Logger::instance().warn(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
#define LOG_DEBUG(msg) Logger::instance().debug(msg)

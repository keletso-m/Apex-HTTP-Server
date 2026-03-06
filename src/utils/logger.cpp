#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::init(const std::string& log_file, LogLevel min_level) {
    min_level_ = min_level;
    if (!log_file.empty()) {
        file_.open(log_file, std::ios::app);
        if (!file_) std::cerr << "[WARN] Could not open log file: " << log_file << "\n";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level_) return;
    std::string entry = "[" + timestamp() + "] [" + level_str(level) + "] " + message;
    std::cout << entry << "\n";
    if (file_.is_open()) file_ << entry << "\n";
}

std::string Logger::level_str(LogLevel l) const {
    switch (l) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return " INFO";
        case LogLevel::WARN:  return " WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "?????";
}

std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

#pragma once
#include <atomic>
#include <chrono>

class Metrics {
public:
    Metrics() : start_time_(std::chrono::steady_clock::now()) {}

    void record_request() {
        total_requests_.fetch_add(1, std::memory_order_relaxed);
    }

    long long total_requests() const {
        return total_requests_.load(std::memory_order_relaxed);
    }

    long long uptime_seconds() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time_).count();
    }

private:
    std::atomic<long long> total_requests_{0};
    std::chrono::steady_clock::time_point start_time_;
};
#pragma once
#include <atomic>
#include <chrono>

class RateLimiter {
public:
    explicit RateLimiter(int max_requests_per_second)
        : max_per_second_(max_requests_per_second),
          window_start_ms_(now_ms()),
          count_(0) {}

    // returns true if this request is allowed, false if the limit was hit.
    bool allow() {
        auto now = now_ms();
        long long window = window_start_ms_.load();

        if (now - window >= 1000) {
            // Window expired,try to reset it
            if (window_start_ms_.compare_exchange_strong(window, now)) {
                count_.store(0);
            }
        }

        return ++count_ <= max_per_second_;
    }

private:
    int max_per_second_;
    std::atomic<long long> window_start_ms_;
    std::atomic<int> count_;

    static long long now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
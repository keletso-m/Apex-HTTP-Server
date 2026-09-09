#pragma once
#include <atomic>
#include <chrono>

class RateLimiter {
public:
    explicit RateLimiter(int max_requests_per_second)
        : max_per_second_(max_requests_per_second),
          window_start_ms_(now_ms()),
          count_(0) {}

    bool allow() {
        auto now = now_ms();
        long long window = window_start_ms_.load(std::memory_order_relaxed);

        // Reset window if expired
        if (now - window >= 1000) {
            long long expected = window;
            if (window_start_ms_.compare_exchange_strong(
                    expected, now,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                // reset counter
                count_.store(1, std::memory_order_relaxed);
                return true;   // this request is the first in the new window
            }
            // Another thread reset the window, fall through to normal check
        }

        int current = count_.fetch_add(1, std::memory_order_relaxed) + 1;
        return current <= max_per_second_;
    }

private:
    int max_per_second_;
    std::atomic<long long> window_start_ms_;
    std::atomic<int>       count_;

    static long long now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

struct WorkItem {
    int         client_fd;
    std::string client_ip;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads, size_t max_queue_size = 1000);
    ~ThreadPool();

    // Returns false if queue is full (caller should reject with 503)
    bool enqueue(WorkItem item);

    void stop();          // drain queue then shut down
    void set_handler(std::function<void(WorkItem)> handler);

private:
    std::vector<std::thread>   workers_;
    std::queue<WorkItem>       queue_;
    std::mutex                 mutex_;
    std::condition_variable    cv_;
    std::atomic<bool>          stopping_{ false };
    size_t                     max_queue_size_;
    std::function<void(WorkItem)> handler_;

    void worker_loop();
};
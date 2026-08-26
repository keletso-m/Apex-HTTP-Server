#include "thread_pool.h"
#include "logger.h"
#include <unistd.h>

ThreadPool::ThreadPool(size_t num_threads, size_t max_queue_size)
    : max_queue_size_(max_queue_size)
{
    for (size_t i = 0; i < num_threads; ++i)
        workers_.emplace_back(&ThreadPool::worker_loop, this);

    LOG_INFO("ThreadPool started with " + std::to_string(num_threads) + " workers");
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::set_handler(std::function<void(WorkItem)> handler) {
    handler_ = std::move(handler);
}

bool ThreadPool::enqueue(WorkItem item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() >= max_queue_size_) {
        LOG_WARN("Queue full — rejecting connection from " + item.client_ip);
        return false;   // caller closes fd and optionally sends 503
    }
    queue_.push(std::move(item));
    lock.unlock();
    cv_.notify_one();
    return true;
}

void ThreadPool::stop() {
    stopping_ = true;
    cv_.notify_all();           // wake all workers so they can exit
    for (auto& t : workers_)
        if (t.joinable()) t.join();
    workers_.clear();
}

void ThreadPool::worker_loop() {
    while (true) {
        WorkItem item;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return !queue_.empty() || stopping_;
            });

            if (stopping_ && queue_.empty()) return;
            item = std::move(queue_.front());
            queue_.pop();
        }
        // Handle outside the lock
        if (handler_) {
            try {
                handler_(item);
            } catch (const std::exception& e) {
                LOG_ERROR("Unhandled exception in worker: " + std::string(e.what())
                          + " — closing connection fd=" + std::to_string(item.client_fd));
                close(item.client_fd);
            } catch (...) {
                LOG_ERROR("Unknown exception in worker — closing connection fd="
                          + std::to_string(item.client_fd));
                close(item.client_fd);
            }
        } else {
            close(item.client_fd);
        }
    }
}
#pragma once // header guard to include header only once per compilation unit 
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>  // thread synchonisation
#include <functional>
#include <atomic>
#include <string>

struct WorkItem{
    int client_fp;
    std::string client_ip;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t num_thread, size_t max_queue_size);
    ~ThreadPool();  // return false if the queue is full  caler should reject with 503
    bool enqueue(WorkItem item);
    void stop();   // drain queue athen shut down the pool
    void set_handler(std::function<void(WorkItem)> handler);
    
private:
    std:: vector<std::thread>  workers_;
    std::queue<WorkItem>   queue_;
    std::mutex   mutex_;
    std::condition_variable  cv_;
    std::atomic<bool>  stopping_{ false};
    size_t  max_queue_size_t_;
    std::function<void(WorkItem)> hnandler;

    void worker_loop();
};
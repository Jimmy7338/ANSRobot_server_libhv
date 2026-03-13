#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <memory>

class ThreadPool {
public:
    // 构造函数：指定工作线程数
    explicit ThreadPool(size_t num_threads = 4);
    
    // 析构函数：等待所有任务完成
    ~ThreadPool();
    
    // 提交任务（无返回值）
    template<typename F, typename... Args>
    void submit(F&& f, Args&&... args) {
        auto task = std::make_shared<std::function<void()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (shutdown_) {
                throw std::runtime_error("ThreadPool is shutdown");
            }
            task_queue_.push(task);
        }
        
        condition_.notify_one();
    }
    
    // 获取队列大小
    size_t queue_size() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return task_queue_.size();
    }
    
    // 获取工作线程数
    size_t num_threads() const { return num_threads_; }
    
    // 等待所有任务完成
    void wait_all();
    
private:
    void worker_thread();
    
    size_t num_threads_;
    std::vector<std::thread> threads_;
    std::queue<std::shared_ptr<std::function<void()>>> task_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool shutdown_ = false;
};

#endif // THREAD_POOL_H

#pragma once

#include <functional>
#include <thread>


class ThreadPool {
public:
    ThreadPool(size_t threadCount) {
        threads_.reserve(threadCount);
        for (size_t i = 0; i < threadCount; i++) {
            threads_.emplace_back([&] {
                while (!terminating_) {
                    std::unique_lock lock(mtx_);
                    queueCv_.wait(
                        lock, [&] { return !tasks_.empty() || terminating_; });

                    if (terminating_) break;

                    std::function<void()> task = tasks_.front();
                    tasks_.pop();
                    if (tasks_.empty()) termCv_.notify_one();
                    lock.unlock();

                    task();
                }
            });
        }
    }

    void PushTask(const std::function<void()>& task) {
        if (!accepting_)
            throw std::logic_error("Can't push a task to terminated pool.");
        std::unique_lock lock(mtx_);
        tasks_.push(task);
        queueCv_.notify_one();
    }

    void Terminate(bool wait) {
        accepting_ = false;
        if (wait) {
            std::unique_lock lock(mtx_);
            if (!tasks_.empty()) termCv_.wait(lock);

            terminating_ = true;
        } else {
            terminating_ = true;

            std::unique_lock lock(mtx_);
            while (!tasks_.empty()) tasks_.pop();
        }

        queueCv_.notify_all();
        for (auto& t : threads_) t.join();
    }

    bool IsActive() const { return accepting_; }

    size_t QueueSize() const {
        std::unique_lock lock(mtx_);
        return tasks_.size();
    }

private:
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mtx_;
    std::condition_variable queueCv_, termCv_;

    std::vector<std::thread> threads_;
    std::atomic<bool> accepting_ = true, terminating_ = false;
};

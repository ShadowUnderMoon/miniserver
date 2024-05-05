#pragma once

#include <memory>
#include <mutex>
#include <queue>

template<typename T>
class threadsafe_queue {
public:
    threadsafe_queue() = default;

    threadsafe_queue(const threadsafe_queue&) = delete;
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;
    threadsafe_queue(threadsafe_queue&&) = default;
    threadsafe_queue& operator=(threadsafe_queue&&) = default;    
    void enqueue(T&& item) {
        std::unique_lock<std::mutex> lock(*queue_mutex_);
        queue_.push(std::move(item));
    }

    T dequeue() {
        std::unique_lock<std::mutex> lock(*queue_mutex_);
        auto poppped_item = std::move(queue_.front());
        queue_.pop();
        return poppped_item;
    }
private:
    std::unique_ptr<std::mutex> queue_mutex_ = std::make_unique<std::mutex>();
    std::queue<T> queue_;
};
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

class thread_pool;
template <typename T>
class mpmc_blocking_queue {
public:
    friend class thread_pool;
    using item_type = T;
    explicit mpmc_blocking_queue(size_t max_items)
        : max_items_(max_items) {}

    void enqueue(T&& item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            pop_cv_.wait(lock, [this] { return this->q_.size() != max_items_; });
            q_.push(std::move(item));
        }
        push_cv_.notify_one();
    }

    void enqueue_nowait(T&& item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (q_.size() == max_items_ && q_.size() > 0) {
                q_.pop();
                ++overrun_counter_;
            }
            q_.push(std::move(item));
        }
        push_cv_.notify_one();
    }

    void enqueue_if_have_room(T&& item) {
        bool pushed = false;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (q_.size() != max_items_) {
                q_.push(std::move(item));
                pushed = true;
            }
        }
        if (pushed) {
            push_cv_.notify_one();
        } else {
            ++discard_counter_;
        }
    }

    bool dequeue_for(T& popped_item, std::chrono::milliseconds wait_duration) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (!push_cv_.wait_for(lock, wait_duration, [this] { return !q_.empty(); })) {
                return false;
            }
            popped_item = std::move(q_.front());
            q_.pop();
        }
        pop_cv_.notify_one();
        return true;
    }

    void dequeue(T& popped_item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            push_cv_.wait(lock, [this] { return !q_.empty(); });
            popped_item = std::move(q_.front());
            q_.pop();
        }
        pop_cv_.notify_one();
    }

    size_t overrun_counter() {
        return overrun_counter_ .load(std::memory_order_relaxed);
    }
    size_t discard_counter() {
        return discard_counter_.load(std::memory_order_relaxed);
    }
    size_t size() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return q_.size();
    }
    void reset_overrun_counter() {
        overrun_counter_.store(0, std::memory_order_relaxed);
    }
    void reset_discard_counter() {
        discard_counter_.store(0, std::memory_order_relaxed);
    }
private:
    std::mutex queue_mutex_;
    std::condition_variable push_cv_;
    std::condition_variable pop_cv_;
    std::queue<T> q_;
    size_t max_items_{0};
    std::atomic<size_t> discard_counter_{0};
    std::atomic<size_t> overrun_counter_{0};
    
};
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
        : max_items(max_items) {}

    void enqueue(T&& item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            pop_cv.wait(lock, [this] { return this->q.size() != max_items; });
            q.push(std::move(item));
        }
        push_cv.notify_one();
    }

    void enqueue_nowait(T&& item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (q.size() == max_items && q.size() > 0) {
                q.pop();
                ++m_overrun_counter;
            }
            q.push(std::move(item));
        }
        push_cv.notify_one();
    }

    void enqueue_if_have_room(T&& item) {
        bool pushed = false;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (q.size() != max_items) {
                q.push(std::move(item));
                pushed = true;
            }
        }
        if (pushed) {
            push_cv.notify_one();
        } else {
            ++m_discard_counter;
        }
    }

    bool dequeue_for(T& popped_item, std::chrono::milliseconds wait_duration) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (!push_cv.wait_for(lock, wait_duration, [this] { return !q.empty(); })) {
                return false;
            }
            popped_item = std::move(q.front());
            q.pop();
        }
        pop_cv.notify_one();
        return true;
    }

    void dequeue(T& popped_item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            push_cv.wait(lock, [this] { return !q.empty(); });
            popped_item = std::move(q.front());
            q.pop();
        }
        pop_cv.notify_one();
    }

    size_t overrun_counter() {
        return m_overrun_counter .load(std::memory_order_relaxed);
    }
    size_t discard_counter() {
        return m_discard_counter.load(std::memory_order_relaxed);
    }
    size_t size() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return q.size();
    }
    void reset_overrun_counter() {
        m_overrun_counter.store(0, std::memory_order_relaxed);
    }
    void reset_discard_counter() {
        m_discard_counter.store(0, std::memory_order_relaxed);
    }
private:
    std::mutex queue_mutex;
    std::condition_variable push_cv;
    std::condition_variable pop_cv;
    std::queue<T> q;
    size_t max_items{0};
    std::atomic<size_t> m_discard_counter{0};
    std::atomic<size_t> m_overrun_counter{0};
    
};
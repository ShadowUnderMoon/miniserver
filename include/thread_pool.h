#pragma once

#include <cassert>
#include <memory>
#include <mpmc_blocking_q.h>
#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

enum class async_overflow_policy {
    block,
    overrun_oldest,
    discard_new
};

enum class async_msg_type {normal, terminate};

class async_msg {
public:
    async_msg_type msg_type{async_msg_type::normal};
    std::function<void()> task;

    async_msg() = default;
    ~async_msg() = default;

    async_msg(const async_msg &) = delete;
    async_msg(async_msg &&) = default;
    async_msg& operator=(async_msg &&) = default;

    explicit async_msg(std::function<void()>&& task_)
        : task(std::move(task_)) {}
    explicit async_msg(async_msg_type the_type)
        : msg_type(the_type) {}
};

class thread_pool {
public:
    using item_type = async_msg;
    using q_type = mpmc_blocking_queue<item_type>;
    thread_pool(size_t q_max_items,
                size_t threads_n,
                std::function<void()> on_thread_start,
                std::function<void()> on_thread_stop)
        : q_(q_max_items)
    {
        if (threads_n == 0 || threads_n > 1000) {
            throw std::runtime_error("invalid threads_n params (range is 1-1000)");
        }
        for (size_t i = 0; i < threads_n; i++) {
            threads_.emplace_back([this, on_thread_start, on_thread_stop] {
                on_thread_start();
                this->worker_loop_();
                on_thread_stop();
            });
        }
    }

    thread_pool(size_t q_max_items,
                size_t threads_n,
                std::function<void()> on_thread_start)
        : thread_pool(q_max_items, threads_n, on_thread_start, [] {}) {}
    
    thread_pool(size_t q_max_items, size_t threads_n)
        : thread_pool(q_max_items, threads_n, [] {}, [] {}) {}
    
    ~thread_pool() {
        for (size_t i = 0; i < threads_.size(); i++) {
            post_async_msg_(async_msg(async_msg_type::terminate), async_overflow_policy::block);
        }

    }

    thread_pool(const thread_pool &) = delete;
    thread_pool& operator=(thread_pool &&) = delete;

    void post_log(std::function<void()>&& f,
                  async_overflow_policy overflow_policy)
    {
        async_msg async_m(std::move(f));
        post_async_msg_(std::move(async_m), overflow_policy);
    }


    size_t overrun_counter() {
        return q_.overrun_counter();
    }

    void reset_overrun_counter() {
        q_.reset_overrun_counter();
    }

    size_t discard_counter() {
        return q_.discard_counter();
    }

    void reset_discard_counter() {
        q_.reset_discard_counter();
    }

    size_t queue_size() {
        return q_.size();
    }

private:
    q_type q_;
    std::vector<std::jthread> threads_;

    void post_async_msg_(async_msg&& new_msg, async_overflow_policy overflow_policy = async_overflow_policy::block) {
        if (overflow_policy == async_overflow_policy::block) {
            q_.enqueue(std::move(new_msg));
        } else if (overflow_policy == async_overflow_policy::overrun_oldest) {
            q_.enqueue_nowait(std::move(new_msg));
        } else {
            assert(overflow_policy == async_overflow_policy::discard_new);
            q_.enqueue_if_have_room(std::move(new_msg));
        }
    }

    void worker_loop_() {
        while (process_next_msg_()) {

        }
    }

    bool process_next_msg_() {
        async_msg incoming_async_msg;
        q_.dequeue(incoming_async_msg);
        if (incoming_async_msg.msg_type == async_msg_type::terminate) {
            return false;
        }
        incoming_async_msg.task();
        return true;
    }
};
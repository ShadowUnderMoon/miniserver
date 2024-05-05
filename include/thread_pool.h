#pragma once

#include <notify_event_fd.h>
#include <epoller.h>
#include <threadsafe_queue.h>
#include <sys/eventfd.h>
#include <cassert>
#include <cstddef>
#include <functional>
#include <mpmc_blocking_q.h>
#include <stdexcept>
#include <thread>
#include <vector>
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

    explicit async_msg(std::function<void()>&& task)
        : task(std::move(task)) {}
    explicit async_msg(async_msg_type the_type)
        : msg_type(the_type) {}
};

class WorkerThread {
public:
    WorkerThread(std::function<void(void)> func)
        : thread_(func) {}

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = default;
    WorkerThread& operator=(WorkerThread&&) = default;
private:
    std::jthread thread_;
    NotifyEventFd notify_event_fd_;
    threadsafe_queue<async_msg> conn_queue_;
    Epoller epller_;
};

class WorkerThreadPool {
public:
    WorkerThreadPool(size_t threads_n,
                const std::function<void()>& on_thread_start,
                const std::function<void()>& on_thread_stop)
    {
        if (threads_n == 0 || threads_n > 1000) {
            throw std::runtime_error("invalid threads_n params (range is 1-1000)");
        }
        for (size_t i = 0; i < threads_n; i++) {
            worker_threads_.emplace_back([] () {});
        }
    }
    
    explicit WorkerThreadPool(size_t threads_n)
        : WorkerThreadPool(threads_n, [] {}, [] {}) {}
    
    ~WorkerThreadPool() {

    }

    WorkerThreadPool(const WorkerThreadPool &) = delete;
    WorkerThreadPool& operator=(WorkerThreadPool &&) = delete;
private:
    std::vector<WorkerThread> worker_threads_;
};

class thread_pool {
public:
    using item_type = async_msg;
    using q_type = mpmc_blocking_queue<item_type>;
    thread_pool(size_t q_max_items,
                size_t threads_n,
                const std::function<void()>& on_thread_start,
                const std::function<void()>& on_thread_stop)
        : q(q_max_items)
    {
        if (threads_n == 0 || threads_n > 1000) {
            throw std::runtime_error("invalid threads_n params (range is 1-1000)");
        }
        for (size_t i = 0; i < threads_n; i++) {
            threads.emplace_back([this, on_thread_start, on_thread_stop] {
                on_thread_start();
                this->worker_loop();
                on_thread_stop();
            });
        }
    }

    thread_pool(size_t q_max_items,
                size_t threads_n,
                const std::function<void()>& on_thread_start)
        : thread_pool(q_max_items, threads_n, on_thread_start, [] {}) {}
    
    thread_pool(size_t q_max_items, size_t threads_n)
        : thread_pool(q_max_items, threads_n, [] {}, [] {}) {}
    
    ~thread_pool() {
        for (size_t i = 0; i < threads.size(); i++) {
            post_async_msg(async_msg(async_msg_type::terminate), async_overflow_policy::block);
        }

    }

    thread_pool(const thread_pool &) = delete;
    thread_pool& operator=(thread_pool &&) = delete;

    void add_task(std::function<void()>&& f,
                  async_overflow_policy overflow_policy = async_overflow_policy::block)
    {
        async_msg async_m(std::move(f));
        post_async_msg(std::move(async_m), overflow_policy);
    }


    size_t overrun_counter() {
        return q.overrun_counter();
    }

    void reset_overrun_counter() {
        q.reset_overrun_counter();
    }

    size_t discard_counter() {
        return q.discard_counter();
    }

    void reset_discard_counter() {
        q.reset_discard_counter();
    }

    size_t queue_size() {
        return q.size();
    }

private:
    q_type q;
    std::vector<std::jthread> threads;

    void post_async_msg(async_msg&& new_msg, async_overflow_policy overflow_policy = async_overflow_policy::block) {
        if (overflow_policy == async_overflow_policy::block) {
            q.enqueue(std::move(new_msg));
        } else if (overflow_policy == async_overflow_policy::overrun_oldest) {
            q.enqueue_nowait(std::move(new_msg));
        } else {
            assert(overflow_policy == async_overflow_policy::discard_new);
            q.enqueue_if_have_room(std::move(new_msg));
        }
    }

    void worker_loop() {
        while (process_next_msg()) {

        }
    }

    bool process_next_msg() {
        async_msg incoming_async_msg;
        q.dequeue(incoming_async_msg);
        if (incoming_async_msg.msg_type == async_msg_type::terminate) {
            return false;
        }
        incoming_async_msg.task();
        return true;
    }
};
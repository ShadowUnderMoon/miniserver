#pragma once
#include "socket_descriptor.h"
#include <cerrno>
#include <httpconn.h>
#include <setting.h>
#include <logger.h>
#include <heaptimer.h>
#include <notify_event_fd.h>
#include <epoller.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
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

template <class T>
class async_msg {
public:
    async_msg_type msg_type{async_msg_type::normal};
    T msg;

    async_msg() = default;
    ~async_msg() = default;

    async_msg(const async_msg &) = delete;
    async_msg& operator=(const async_msg &) = default;
    async_msg(async_msg &&) = default;
    async_msg& operator=(async_msg &&) = default;

    explicit async_msg(T&& message)
        : msg(std::move(message)) {}
    explicit async_msg(async_msg_type the_type)
        : msg_type(the_type) {}
};

class ServerHandler {
public:
    ServerHandler() = default;
    ServerHandler(const ServerHandler&) = delete;
    ServerHandler& operator=(const ServerHandler&) = delete;
    ServerHandler(ServerHandler&&) = default;
    ServerHandler& operator=(ServerHandler&&) = default;
    void Start() {
       SPDLOG_LOGGER_INFO(logger, "Worker Start!");
        while (true) {
            int timeMs = Setting::GetInstance().timeout == std::chrono::milliseconds(0) ? -1 : timer.GetNextTick(); 
            int eventCnt = epoller.Wait(timeMs);
            if (eventCnt < 0) {
                throw std::system_error(errno, std::generic_category());
            }
            for (int i = 0; i < eventCnt; i++) {
                int fd = epoller.GetEventFd(i);
                uint32_t events = epoller.GetEvents(i);
                if (fd == notify_event_fd.Get()) {
                    SPDLOG_LOGGER_INFO(logger, "Notify Event");
                    DealNotify();
                } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    assert(user.contains(fd));
                    SPDLOG_LOGGER_INFO(logger,  "ERROR/CLOSE EVENT: {}", fd);
                    CloseConn(fd);
                } else if (events & EPOLLIN) {
                    assert(user.contains(fd));
                    SPDLOG_LOGGER_INFO(logger, "READ EVENT: {}", fd);
                    DealRead(fd);
                } else if (events & EPOLLOUT) {
                    assert(user.contains(fd));
                    SPDLOG_LOGGER_INFO(logger, "WRITE EVENT: {}", fd);
                    DealWrite(fd);
                } else {
                    logger->error("Unexpected event");
                }
            }
        }
    }

    void DealNotify() {
        auto num = notify_event_fd.Read();
        for (auto i = 0; i < num; i++) {
            auto message = conn_queue.dequeue();
            AddClient(message.msg);
        }
    }

    void AddClient(Connection conn) {
        auto [client_fd, client_addr] = conn;
        auto sock = std::make_unique<ServerSocket>(client_fd);
        sock->setNonblock();
        epoller.AddFd(client_fd, Setting::GetInstance().conn_event | EPOLLIN);
        user[client_fd].Init(std::move(sock), client_addr);
        if (Setting::GetInstance().timeout > std::chrono::milliseconds(0)) {
            timer.Add(client_fd, Setting::GetInstance().timeout, [this, client_fd] { CloseConn(client_fd); });
        }
        SPDLOG_LOGGER_INFO(logger, "Client {}:{} in", client_fd, sockaddrToString(client_addr));
    }

    void CloseConn(int fd) {
        epoller.DelFd(fd);
        user.erase(fd);
        if (Setting::GetInstance().timeout > std::chrono::milliseconds(0)) {
            timer.Del(fd);
        }
    }

    void DealRead(int fd) {
        if (Setting::GetInstance().timeout > std::chrono::milliseconds(0)) {
            timer.Extend(fd, Setting::GetInstance().timeout);
        }
        auto& client = user[fd];
        int read_errno = 0;
        auto ret = client.Read(read_errno);
        if (ret <= 0 && (read_errno != EAGAIN || read_errno != EWOULDBLOCK)) {
            CloseConn(fd);
            return;
        }
        if (client.Process()) {
            epoller.ModFd(client.GetFd(), Setting::GetInstance().conn_event | EPOLLOUT);
        } else {
            epoller.ModFd(client.GetFd(), Setting::GetInstance().conn_event | EPOLLIN);
        }
    }

    void DealWrite(int fd) {
        if (Setting::GetInstance().timeout > std::chrono::milliseconds(0)) {
            timer.Extend(fd, Setting::GetInstance().timeout);
        }
        int error_no = 0;
        auto& client = user[fd];
        auto ret = client.Write(error_no);
        if (ret < 0) {
            if (error_no == EAGAIN || error_no == EWOULDBLOCK) {
                epoller.ModFd(client.GetFd(), Setting::GetInstance().conn_event | EPOLLOUT);
            } else {
                CloseConn(client.GetFd());
            }
        } else if (client.IsKeepAlive()) {
            epoller.ModFd(client.GetFd(), Setting::GetInstance().conn_event | EPOLLIN);
        } else {
            CloseConn(client.GetFd());
        }
    }
    NotifyEventFd notify_event_fd;
    threadsafe_queue<async_msg<Connection>> conn_queue;
    Epoller epoller;
    HeapTimer timer;
    std::unordered_map<int, HttpConn> user;
};

class WorkerThread {
public:
    WorkerThread() = default;

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = default;
    WorkerThread& operator=(WorkerThread&&) = default;

    std::jthread thread;
    ServerHandler serverhandler;
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
        worker_threads.resize(threads_n);
        for (size_t i = 0; i < threads_n; i++) {
            auto& current_worker = worker_threads[i];
            current_worker.serverhandler.epoller.AddFd(current_worker.serverhandler.notify_event_fd.Get(), EPOLLIN);
            current_worker.thread = std::jthread([&current_worker, on_thread_start, on_thread_stop] () {
                on_thread_start();
                current_worker.serverhandler.Start(); 
                on_thread_stop();
            });
        }
    }
    
    explicit WorkerThreadPool(size_t threads_n)
        : WorkerThreadPool(threads_n, [] {}, [] {}) {}
    
    ~WorkerThreadPool() {

    }

    WorkerThreadPool(const WorkerThreadPool &) = delete;
    WorkerThreadPool& operator=(WorkerThreadPool &&) = delete;
    std::vector<WorkerThread> worker_threads;
};

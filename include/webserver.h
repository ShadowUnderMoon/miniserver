#pragma once

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <ratio>
#include <spdlog/common.h>
#include <string>
#include "heaptimer.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <magic_enum.hpp>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <epoller.h>
#include <httpconn.h>
#include <socket_descriptor.h>
#include <sys/epoll.h>
#include <thread_pool.h>
#include <unistd.h>
#include <sqlconnpool.h>
class WebServer {
  public:
    WebServer(
        int port,
        bool ET,
        std::chrono::milliseconds timeout,
        bool optLinger,
        std::string work_dir,
        std::string db_name,
        std::string db_server,
        std::string db_user,
        std::string db_password,
        unsigned int db_port,
        std::chrono::seconds db_max_idle_time,
        int db_initial_connections,
        int db_max_connections,
        int connPoolNum,
        int threadNum,
        bool openLog,
        spdlog::level::level_enum logLevel,
        int logQueSize)
        : isET_(ET), timeout_(timeout), threadpool_(std::make_unique<thread_pool>(10000, threadNum)) {
        HttpConn::src_dir = work_dir + "/resources";
        initEventMode(ET);
        listen_sock_ =
            std::make_unique<ServerSocket>(ServerSocket::get_new_scoket());
        listen_sock_->listenTo(port, 10);

        epoller_.AddFd(listen_sock_->get(), listen_event_);

        MysqlConnectionPool::GetInstance().Init(
            db_name, db_server, db_user, db_password, db_port, db_max_idle_time,
            db_initial_connections, db_max_connections);
        // log configuration
        spdlog::init_thread_pool(logQueSize, 1);
        auto stdout_sink =
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            "logs/miniserver.log", true);
        std::vector<spdlog::sink_ptr> sinks{stdout_sink, file_sink};
        logger_ = std::make_shared<spdlog::async_logger>(
            "miniserver", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
        logger_->set_pattern("%l %Y-%m-%d %H:%M:%S.%e [%s:%#:%!] %^%v%$");
        if (!openLog) {
            logger_->set_level(spdlog::level::off);
        } else {
            logger_->set_level(logLevel);
        }
        spdlog::register_logger(logger_);
        SPDLOG_LOGGER_INFO(logger_, "hello");
        SPDLOG_LOGGER_INFO(logger_, "MiniServer Configuration:");
        SPDLOG_LOGGER_INFO(
            logger_, "port: {}, ET: {}, linger: {}, curSrc: {}", port, ET,
            optLinger, HttpConn::src_dir);
        SPDLOG_LOGGER_INFO(
            logger_, "DB info: name: {}, server: {}, user: {}, port: {}, max_idle_time: {} s, initial connection: {}, max connection: {}",
            db_name, db_server, db_user, db_port, db_max_idle_time.count(), db_initial_connections, db_max_connections);
        SPDLOG_LOGGER_INFO(
            logger_, "logLevel: {}, logQueueSize: {}",
            magic_enum::enum_name(logger_->level()), logQueSize);
        SPDLOG_LOGGER_INFO(
            logger_, "SqlConnPool: {}, ThreadPoolNum: {}", connPoolNum,
            threadNum);
    }

    ~WebServer() { SPDLOG_LOGGER_INFO(logger_, "MiniServer End!"); }

    void initEventMode(bool ET) {
        listen_event_ = EPOLLRDHUP | EPOLLIN;
        conn_event_ = EPOLLONESHOT | EPOLLRDHUP;

        if (ET) {
            listen_event_ |= EPOLLET;
            conn_event_ |= EPOLLET;
        }

        HttpConn::isET = (conn_event_ & EPOLLET);
    }

    void Start() {
        SPDLOG_LOGGER_INFO(logger_, "MiniServer Start!");

        while (!is_close_) {
            int timeMs = timeout_ == std::chrono::milliseconds(0) ? -1 : timer_.GetNextTick(); 
            int eventCnt = epoller_.Wait(timeMs);
            if (eventCnt < 0) {
                throw std::system_error(errno, std::generic_category());
            }
            for (int i = 0; i < eventCnt; i++) {
                int fd = epoller_.GetEventFd(i);
                uint32_t events = epoller_.GetEvents(i);
                if (fd == listen_sock_->get()) {
                    SPDLOG_LOGGER_INFO(logger_, "LISTEN EVENT");
                    DealListen();
                } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    assert(user_.contains(fd));
                    SPDLOG_LOGGER_INFO(logger_, "ERROR/CLOSE EVENT: {}", fd);
                    CloseConn(fd);
                } else if (events & EPOLLIN) {
                    assert(user_.contains(fd));
                    SPDLOG_LOGGER_INFO(logger_, "READ EVENT: {}", fd);
                    DealRead(fd);
                } else if (events & EPOLLOUT) {
                    assert(user_.contains(fd));
                    SPDLOG_LOGGER_INFO(logger_, "WRITE EVENT: {}", fd);
                    DealWrite(fd);
                } else {
                    logger_->error("Unexpected event");
                }
            }
        }
    }

    void AddClient(int fd, sockaddr_in addr) {
        auto sock = std::make_unique<ServerSocket>(fd);
        sock->setNonblock();
        epoller_.AddFd(fd, conn_event_ | EPOLLIN);
        user_.emplace(fd, HttpConn(std::move(sock), addr));
        if (timeout_ > std::chrono::milliseconds(0)) {
            timer_.Add(fd, timeout_, [this, fd] { CloseConn(fd); });
        }
        SPDLOG_LOGGER_INFO(
            logger_, "Client {}:{} in", fd, sockaddrToString(addr));
    }

    static std::string sockaddrToString(const struct sockaddr_in &addr) {
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ipstr, INET_ADDRSTRLEN);
        return std::string(ipstr);
    }

    void DealListen() {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        do {
            int client_fd = accept(
                listen_sock_->get(), (struct sockaddr *)&client_addr,
                &client_addr_len);
            if (client_fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
                throw std::system_error(errno, std::generic_category());
            }
            if (HttpConn::user_count >= MAX_FD) {
                SPDLOG_LOGGER_WARN(
                    spdlog::get("miniserver"), "Client if full!");
                SendError(client_fd, "Server Busy!");
                return;
            }
            AddClient(client_fd, client_addr);
        } while (listen_event_ & EPOLLET);
    }

    void SendError(int fd, const std::string &info) {
        size_t ret = send(fd, info.data(), info.length(), 0);
        if (ret < 0) {
            SPDLOG_LOGGER_WARN(
                spdlog::get("miniserver"), "send error to client [{}] error!",
                fd);
        }
        close(fd);
    }

    void CloseConn(int fd) {
        epoller_.DelFd(fd);
        user_.erase(fd);
        if (timeout_ > std::chrono::milliseconds(0)) {
            timer_.Del(fd);
        }
        close(fd);
    }

    void DealRead(int fd) {
        if (timeout_ > std::chrono::milliseconds(0)) {
            timer_.Extend(fd, timeout_);
        }
        // OnRead(user_[fd]); 
        threadpool_->add_task([this, fd] { OnRead(user_[fd]); });
    }

    void OnRead(HttpConn &client) {
        client.Read();
        OnProcess(client);
    }

    void OnProcess(HttpConn &client) {
        if (client.Process()) {
            epoller_.ModFd(client.GetFd(), conn_event_ | EPOLLOUT);
        } else {
            epoller_.ModFd(client.GetFd(), conn_event_ | EPOLLIN);
        }
    }

    void DealWrite(int fd) {
        if (timeout_ > std::chrono::milliseconds(0)) {
            timer_.Extend(fd, timeout_);
        }
        threadpool_->add_task([this, fd] { OnWrite(user_[fd]); });
        // OnWrite(user_[fd]);
    }

    void OnWrite(HttpConn &client) {
        client.Write();
        if (client.ToWriteBytes() == 0) {
            if (client.IsKeepAlive()) {
                OnProcess(client);
                return;
            }
            CloseConn(client.GetFd());
            return;
        }
        epoller_.ModFd(client.GetFd(), conn_event_ | EPOLLOUT);
    }

  private:
    std::chrono::milliseconds timeout_;
    HeapTimer timer_;
    bool is_close_ = false;
    std::unique_ptr<thread_pool> threadpool_;
    std::unique_ptr<ServerSocket> listen_sock_;
    Epoller epoller_;
    uint32_t listen_event_;
    uint32_t conn_event_;
    bool isET_ = true;
    static const int MAX_FD = 65536;
    std::filesystem::path curr_dir_;
    std::shared_ptr<spdlog::async_logger> logger_;
    std::unordered_map<int, HttpConn> user_;
    async_overflow_policy overflow_policy_;
};
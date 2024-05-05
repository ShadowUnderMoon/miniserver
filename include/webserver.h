#pragma once

#include <spdlog/sinks/stdout_color_sinks.h>
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <epoller.h>
#include <httpconn.h>
#include <logger.h>
#include <magic_enum.hpp>
#include <memory>
#include <netinet/in.h>
#include <netinet/in.h>
#include <setting.h>
#include <socket_descriptor.h>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <sqlconnpool.h>
#include <string>
#include <sys/epoll.h>
#include <thread_pool.h>
#include <unistd.h>
class WebServer {
  public:
    WebServer()
        : threadpool_(std::make_unique<WorkerThreadPool>(
            Setting::GetInstance().num_threads)) {
        auto &setting = Setting::GetInstance();
        HttpConn::src_dir = setting.work_dir + "/resources";
        HttpConn::isET = Setting::GetInstance().isET;
        listen_sock_ =
            std::make_unique<ServerSocket>(ServerSocket::get_new_scoket());
        listen_sock_->listenTo(setting.port, 10);

        epoller_.AddFd(listen_sock_->get(), setting.listen_event);

        MysqlConnectionPool::GetInstance().Init(
            setting.db_name, setting.db_server, setting.db_user,
            setting.db_password, setting.db_port, setting.db_max_idle_time,
            setting.db_initial_connections, setting.db_max_connections);
        // log configuration

        if (!setting.openLog) {
            setting.logLevel = spdlog::level::off;
            logger = spdlog::get("");
            logger->set_level(setting.logLevel);
        } else {
            spdlog::init_thread_pool(setting.logQueSize, 1);
            auto stdout_sink =
                std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto file_sink =
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    "logs/miniserver.log", true);
            std::vector<spdlog::sink_ptr> sinks{stdout_sink, file_sink};
            logger = std::make_shared<spdlog::async_logger>(
                "miniserver", sinks.begin(), sinks.end(), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            logger->set_pattern("%l %Y-%m-%d %H:%M:%S.%e [%s:%#:%!] %^%v%$");
            logger->set_level(setting.logLevel);
        }

        logger->info("Server Hello");
        logger->info("MiniServer Configuration:");
        logger->info(
            "port: {}, ET: {}, linger: {}, curSrc: {}", setting.port,
            setting.isET, setting.optLinger, HttpConn::src_dir);
        logger->info(
            "DB info: name: {}, server: {}, user: {}, port: {}, max_idle_time: {} s, initial connection: {}, max connection: {}",
            setting.db_name, setting.db_server, setting.db_user,
            setting.db_port, setting.db_max_idle_time.count(),
            setting.db_initial_connections, setting.db_max_connections);
        logger->info(
            "logLevel: {}, logQueueSize: {}",
            magic_enum::enum_name(logger->level()), setting.logQueSize);
        logger->info(
            "SqlConnPool: {}, ThreadPoolNum: {}", setting.connPoolNum,
            setting.num_threads);
    }

    ~WebServer() { logger->info("MiniServer End!"); }

    void Main() {
        logger->info("MiniServer Start!");

        while (!is_close_) {
            int eventCnt = epoller_.Wait();
            for (int i = 0; i < eventCnt; i++) {
                int fd = epoller_.GetEventFd(i);
                uint32_t events = epoller_.GetEvents(i);
                if (fd == listen_sock_->get()) {
                    logger->info("LISTEN EVENT");
                    DealListen();
                } else {
                    logger->error("Unexpected event");
                }
            }
        }
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
            if (HttpConn::user_count >= Setting::GetInstance().MaxFd) {
                logger->info("Client if full!");
                SendError(client_fd, "Server Busy!");
                return;
            }
            DispatchConnNew({client_fd, client_addr});
        } while (Setting::GetInstance().listen_event & EPOLLET);
    }

    void DispatchConnNew(Connection conn) {
        int worker_id = SelectThreadRoundRobin();
        auto &worker = threadpool_->worker_threads[worker_id];
        async_msg<Connection> message(std::move(conn));
        worker.serverhandler.conn_queue.enqueue(std::move(message));
        worker.serverhandler.notify_event_fd.Write(1);
    }

    int SelectThreadRoundRobin() {
        int tid = (last_thread_ + 1) % Setting::GetInstance().num_threads;
        last_thread_ = tid;
        return tid;
    }
    void SendError(int fd, const std::string &info) {
        size_t ret = send(fd, info.data(), info.length(), 0);
        if (ret < 0) { logger->error("send error to client [{}] error!", fd); }
    }

  private:
    std::chrono::milliseconds timeout_;
    bool is_close_ = false;
    std::unique_ptr<WorkerThreadPool> threadpool_;
    std::unique_ptr<ServerSocket> listen_sock_;
    Epoller epoller_;
    int last_thread_ = -1;
};
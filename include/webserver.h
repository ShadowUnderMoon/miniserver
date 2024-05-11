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
#include <spdlog/spdlog.h>
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
        listen_sock_->listenTo(setting.port, setting.backlog);

        epoller_.AddFd(listen_sock_->get(), setting.listen_event);

        MysqlConnectionPool::GetInstance().Init(
            setting.db_name, setting.db_server, setting.db_user,
            setting.db_password, setting.db_port, setting.db_max_idle_time,
            setting.db_initial_connections, setting.db_max_connections);

        SPDLOG_LOGGER_INFO(logger, "MiniServer Configuration:");
        SPDLOG_LOGGER_INFO(logger, 
            "port: {}, ET: {}, linger: {}, curSrc: {}", setting.port,
            setting.isET, setting.optLinger, HttpConn::src_dir);
        SPDLOG_LOGGER_INFO(logger, 
            "DB info: name: {}, server: {}, user: {}, port: {}, max_idle_time: {} s, initial connection: {}, max connection: {}",
            setting.db_name, setting.db_server, setting.db_user,
            setting.db_port, setting.db_max_idle_time.count(),
            setting.db_initial_connections, setting.db_max_connections);
        SPDLOG_LOGGER_INFO(logger, 
            "logLevel: {}, logQueueSize: {}",
            magic_enum::enum_name(logger->level()), setting.logQueSize);
        SPDLOG_LOGGER_INFO(logger, 
            "SqlConnPool: {}, ThreadPoolNum: {}", setting.db_initial_connections,
            setting.num_threads);
    }

    ~WebServer() { SPDLOG_LOGGER_INFO(logger, "MiniServer End!"); }

    void Main() {
        SPDLOG_LOGGER_INFO(logger, "MiniServer Start!");

        while (!is_close_) {
            int eventCnt = epoller_.Wait();
            for (int i = 0; i < eventCnt; i++) {
                int fd = epoller_.GetEventFd(i);
                uint32_t events = epoller_.GetEvents(i);
                if (fd == listen_sock_->get()) {
                    SPDLOG_LOGGER_INFO(logger, "LISTEN EVENT");
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
                if (errno == EMFILE) {
                    SPDLOG_LOGGER_ERROR(logger, "The per process limit on the number of open file descriptors has been reached");
                    return;
                }
                throw std::system_error(errno, std::generic_category());
            }
            if (HttpConn::user_count >= Setting::GetInstance().MaxFd) {
                SPDLOG_LOGGER_INFO(logger, "Client is full!");
                ServerBusy(client_fd);
                close(client_fd);
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
    void ServerBusy(int fd) {

        std::string response =  "HTTP/1.1 503 Service Unavailable\r\n"
                                "Content-Type: text/html; charset=utf-8\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "<!DOCTYPE html>\r\n"
                                "<html lang=\"en\">\r\n"
                                "<head>\r\n"
                                "    <meta charset=\"UTF-8\">\r\n"
                                "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
                                "    <title>Service Unavailable</title>\r\n"
                                "</head>\r\n"
                                "<body>\r\n"
                                "    <h1>503 Service Unavailable</h1>\r\n"
                                "    <p>The server is currently unable to handle the request due to a temporary overload or maintenance of the server. Please try again later.</p>\r\n"
                                "</body>\r\n"
                                "</html>\r\n";

        size_t ret = send(fd, response.data(), response.length(), 0);
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
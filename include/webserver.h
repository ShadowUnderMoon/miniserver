#pragma once

#include <cstdint>
#include <filesystem>
#include <spdlog/common.h>
#include <string>
#include <memory>

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <magic_enum.hpp>
#include <sys/epoll.h>
#include <system_error>

#include <socket_descriptor.h>
#include <epoller.h>
#include <httpconn.h>

class WebServer {
public:
    WebServer(
        int port, bool ET, int timeoutMs, bool optLinger,
        int sqlPort, std::string sqlUser, std::string sqlPwd, std::string dbName,
        int connPoolNum, int threadNum,
        bool openLog, spdlog::level::level_enum logLevel, int logQueSize):
        ET_(ET) {

        auto currentDir = std::filesystem::current_path();
        initEventMode(ET_);
        ServerSocket listenSock;
        listenSock.listenTo(port, 1000);
        listenFd_ = listenSock.get();

        // log configuration
        spdlog::init_thread_pool(logQueSize, 1);
        auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/miniserver.log", true);
        std::vector<spdlog::sink_ptr> sinks {stdout_sink, file_sink};
        auto logger = std::make_shared<spdlog::async_logger>("miniserver", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        if (!openLog) {
            logger->set_level(spdlog::level::off);
        } else {
            logger->set_level(logLevel);
        }
        spdlog::register_logger(logger);
        logger_ = std::move(logger);
        logger_->info("MiniServer Configuration:");
        logger_->info("port: {}, ET: {}, linger: {}", port, ET_, optLinger);
        logger_->info("logLevel: {}, logQueueSize: {}", magic_enum::enum_name(logger_->level()), logQueSize);
        logger_->info("SqlConnPool: {}, ThreadPoolNum: {}", connPoolNum, threadNum);
    }
    
    ~WebServer() {
        logger_->info("MiniServer End!");
    }

    void initEventMode(bool ET) {
        listenEvent_ = EPOLLRDHUP;
        connEvent_ = EPOLLONESHOT | EPOLLRDHUP;

        if (ET) {
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
        }
    }

    void start() {
        logger_->info("MiniServer Start!");

        while (!isClose_) {
            int eventCnt = epoller_.wait(-1);
            for (int i = 0; i < eventCnt; i++) {
                int fd = epoller_.getEventFd(i);
                uint32_t events = epoller_.getEvents(i);
                if (fd == listenFd_) {
                    accept_();
                } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    close_();
                } else if (events & EPOLLIN) {
                    read_();
                } else if (events & EPOLLOUT) {
                    write_();
                } else {
                    logger_->error("Unexpected event");
                }
            }
        }

    }

    void accept_() {
        
    }

    void close_() {

    }

    void read_() {

    }

    void write_() {

    }

private:
    int timeoutMS_;
    bool isClose_ = false;
    int listenFd_;
    Epoller epoller_;
    uint32_t listenEvent_;
    uint32_t connEvent_;
    bool ET_ = true;
    std::filesystem::path currDir_;
    std::shared_ptr<spdlog::async_logger> logger_;
    std::unordered_map<int, httpConn> users_;
};
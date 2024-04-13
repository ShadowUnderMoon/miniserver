#pragma once

#include <spdlog/common.h>
#include <string>
#include <memory>

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMs, bool Linger,
        int sqlPort, std::string sqlUser, std::string sqlPwd, std::string dbName,
        int connPoolNum, int threadNum,
        bool openLog, spdlog::level::level_enum logLevel, int logQueSize) {

        
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
        logger_ = std::move(logger);

    }
    
    ~WebServer() {
        logger_->info("MiniServer End!");
    }

    void start() {
        logger_->info("MiniServer Start!");

    }

private:
    std::shared_ptr<spdlog::async_logger> logger_;
};
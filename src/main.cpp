#include "setting.h"
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <webserver.h>
using namespace std::chrono_literals;
int main() {
    Setting::GetInstance().Init(
        8888, true, 10min, false,
        "/home/hanj/miniserver", // port, ET, timeoutMs, linger
        std::getenv("MINISERVER_DB_NAME"), std::getenv("MINISERVER_DB_HOST"),
        std::getenv("MINISERVER_DB_USER"),
        std::getenv("MINISERVER_DB_PASSWORD"), 3306, std::chrono::seconds(5), 2,
        10, 12, 6, true, spdlog::level::debug, 1024);

    auto& setting = Setting::GetInstance();
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
    WebServer server;
    server.Main();
}
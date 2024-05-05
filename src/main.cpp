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
        10, 12, 6, false, spdlog::level::off, 1024);
    WebServer server;
    server.Main();
}
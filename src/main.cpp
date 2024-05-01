#include <spdlog/spdlog.h>
#include <webserver.h>
using namespace std::chrono_literals;
int main() {
    WebServer server(
        8888, true, 60000ms, false,
        "/home/hanj/miniserver", // port, ET, timeoutMs, linger
        std::getenv("MINISERVER_DB_NAME"), std::getenv("MINISERVER_DB_HOST"),
        std::getenv("MINISERVER_DB_USER"),
        std::getenv("MINISERVER_DB_PASSWORD"), 3306, std::chrono::seconds(5), 2,
        10, 12, 6, true, spdlog::level::info, 1024);

    server.Start();
}
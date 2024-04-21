#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <webserver.h>
int main() {
    WebServer server(
        7777, true, 60000, false,          // port, ET, timeoutMs, linger
        3306, "hanj", "user", "webserver", // mysql configuration
        12, 6, true, spdlog::level::debug, 1024);

    server.start();
}
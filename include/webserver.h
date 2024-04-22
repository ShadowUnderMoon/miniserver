#pragma once

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <spdlog/common.h>
#include <string>
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
#include <thread_pool.h>
class WebServer {
  public:
    WebServer(
        int port,
        bool ET,
        int /*timeoutMs*/,
        bool optLinger,
        int /*sqlPort*/,
        const std::string & /*sqlUser*/,
        const std::string & /*sqlPwd*/,
        const std::string & /*dbName*/,
        int connPoolNum,
        int threadNum,
        bool openLog,
        spdlog::level::level_enum logLevel,
        int logQueSize)
        : ET(ET), tp(std::make_shared<thread_pool>(10000, threadNum)) {
        auto currentDir = std::filesystem::current_path();
        initEventMode(ET);
        listenSock =
            std::make_unique<ServerSocket>(ServerSocket::get_new_scoket());
        listenSock->listenTo(port, 10);

        epoller.addFd(listenSock->get(), listenEvent);

        // log configuration
        spdlog::init_thread_pool(logQueSize, 1);
        auto stdout_sink =
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            "logs/miniserver.log", true);
        std::vector<spdlog::sink_ptr> sinks{stdout_sink, file_sink};
        logger = std::make_shared<spdlog::async_logger>(
            "miniserver", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
        if (!openLog) {
            logger->set_level(spdlog::level::off);
        } else {
            logger->set_level(logLevel);
        }
        spdlog::register_logger(logger);
        logger = std::move(logger);
        logger->info("MiniServer Configuration:");
        logger->info("port: {}, ET: {}, linger: {}", port, ET, optLinger);
        logger->info(
            "logLevel: {}, logQueueSize: {}",
            magic_enum::enum_name(logger->level()), logQueSize);
        logger->info(
            "SqlConnPool: {}, ThreadPoolNum: {}", connPoolNum, threadNum);
    }

    ~WebServer() { logger->info("MiniServer End!"); }

    void initEventMode(bool ET) {
        listenEvent = EPOLLRDHUP | EPOLLIN;
        connEvent = EPOLLONESHOT | EPOLLRDHUP | EPOLLIN | EPOLLOUT;

        if (ET) {
            listenEvent |= EPOLLET;
            connEvent |= EPOLLET;
        }
    }

    void start() {
        logger->info("MiniServer Start!");

        while (!isClose) {
            int eventCnt = epoller.wait(-1);
            if (eventCnt < 0) {
                throw std::system_error(errno,std::generic_category());
            }
            for (int i = 0; i < eventCnt; i++) {
                int fd = epoller.getEventFd(i);
                uint32_t events = epoller.getEvents(i);
                if (fd == listenSock->get()) {
                    accept();
                } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    assert(users.contains(fd));
                    close(fd);
                } else if (events & EPOLLIN) {
                    assert(users.contains(fd));
                    read(fd);
                } else if (events & EPOLLOUT) {
                    assert(users.contains(fd));
                    write(fd);
                } else {
                    logger->error("Unexpected event");
                }
            }
        }
    }

    void addClient(int fd, sockaddr_in addr) {
        auto sock = std::make_unique<ServerSocket>(fd);
        sock->setNonblock();
        epoller.addFd(fd, connEvent);
        users[fd] = {std::move(sock), addr};
        logger->info("Client {}:{} in", fd, sockaddrToString(addr));
    }

    static std::string sockaddrToString(const struct sockaddr_in& addr) {
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ipstr, INET_ADDRSTRLEN);
        return std::string(ipstr);
    }

    void accept() {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        while (true) {
            int clientSockfd = ::accept(
                listenSock->get(), (struct sockaddr *)&clientAddr,
                &clientAddrLen);
            if (clientSockfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
                throw std::system_error(errno, std::generic_category());
            }
            if (httpConn::userCount >= MAX_FD) {
                // TODO send error
                logger->warn("Clients is full!");
                return;
            }
            addClient(clientSockfd, clientAddr);
        }
    }

    void close(int fd) {
        httpConn &http_conn = users[fd];
        epoller.delFd(fd);
        users.erase(fd);
    }

    void read(int fd) {
        int BUFFER_SIZE = 200;
        char buffer[BUFFER_SIZE];
        int bytesRead = ::read(fd, buffer, sizeof(buffer));
        if (bytesRead < 0) {
           logger->info("read error");
        }

        // 打印客户端发送的消息
        std::cout << "Received message from client: " << buffer << std::endl;

        // 将收到的消息发送回客户端
        int bytesSent = send(fd, buffer, bytesRead, 0);
        if (bytesSent < 0) {
            logger->info("write error");
        }
    }

    void write(int fd) {}

  private:
    int timeoutMS;
    bool isClose = false;
    std::shared_ptr<thread_pool> tp;
    std::unique_ptr<ServerSocket> listenSock;
    Epoller epoller;
    uint32_t listenEvent;
    uint32_t connEvent;
    bool ET = true;
    static const int MAX_FD = 65536;
    std::filesystem::path currDir;
    std::shared_ptr<spdlog::async_logger> logger;
    std::unordered_map<int, httpConn> users;
    async_overflow_policy overflow_policy;
};
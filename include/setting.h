#pragma once

#include <chrono>
#include <cstdint>
#include <spdlog/common.h>
#include <sys/epoll.h>
class Setting {
public:
    static Setting &GetInstance() {
        static Setting setting;
        return setting;
    }

    Setting(const Setting&) = delete;
    Setting& operator=(const Setting&) = delete;

    void Init(
        int port,
        bool isET,
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
        int num_threads,
        bool openLog,
        spdlog::level::level_enum logLevel,
        int logQueSize)
    {
        this->port = port;
        this->isET = isET;
        this->timeout = timeout;
        this->optLinger = optLinger;
        this->work_dir = work_dir;
        this->db_name = db_name;
        this->db_server = db_server;
        this->db_user = db_user;
        this->db_password = db_password;
        this->db_port = db_port;
        this->db_max_idle_time = db_max_idle_time;
        this->db_initial_connections = db_initial_connections;
        this->db_max_connections = db_max_connections;
        this->connPoolNum = connPoolNum;
        this->num_threads = num_threads;
        this->openLog = openLog;
        this->logLevel = logLevel;
        this->logQueSize = logQueSize;
        InitEventMode();
    }

    void InitEventMode() {
        listen_event = EPOLLRDHUP | EPOLLIN;
        conn_event = EPOLLONESHOT | EPOLLRDHUP;

        if (isET) {
            listen_event |= EPOLLET;
            conn_event |= EPOLLET;
        }
    }

    int port;
    bool isET;
    std::chrono::milliseconds timeout;
    bool optLinger;
    std::string work_dir;
    std::string db_name;
    std::string db_server;
    std::string db_user;
    std::string db_password;
    int db_port;
    std::chrono::seconds db_max_idle_time;
    int db_initial_connections;
    int db_max_connections;
    int connPoolNum;
    int num_threads;
    bool openLog;
    spdlog::level::level_enum logLevel;
    int logQueSize;
    uint32_t listen_event;
    uint32_t conn_event;
    const int MaxFd = 65536;
private:
    Setting() = default;
};

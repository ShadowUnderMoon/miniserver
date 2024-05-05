#pragma once
#include <logger.h>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <sys/eventfd.h>
#include <system_error>
#include <unistd.h>
class NotifyEventFd
{
public:
    NotifyEventFd() {
        notify_event_fd_ = eventfd(0, 0);
        SPDLOG_LOGGER_DEBUG(logger, "Notify Event Fd create: {}", notify_event_fd_);
        if (notify_event_fd_ < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    uint64_t Read() const {
        uint64_t u;
        auto s = read(notify_event_fd_, &u, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            throw std::system_error(errno, std::generic_category());
        }
        return u;
    }

    void Write(uint64_t u) const {
        auto s = write(notify_event_fd_, &u, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    int Get() {
        return notify_event_fd_;
    }

    ~NotifyEventFd() {
        SPDLOG_LOGGER_DEBUG(logger, "Notify Event Fd close: {}", notify_event_fd_);
        close(notify_event_fd_);
    }
private:
    int notify_event_fd_ = -1;
};
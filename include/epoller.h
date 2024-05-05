#pragma once

#include <logger.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <system_error>
#include <unistd.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024): events_(maxEvent) {
        epoll_fd_ = epoll_create1(0);
        SPDLOG_LOGGER_DEBUG(logger, "Epoller create: {}", epoll_fd_);
        if (epoll_fd_ < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    Epoller(const Epoller&) = delete;
    Epoller& operator=(const Epoller&) = delete;
    Epoller(Epoller&&) = default;
    Epoller& operator=(Epoller&&) = default;

    ~Epoller() {
        SPDLOG_LOGGER_DEBUG(logger, "Epoller close: {}", epoll_fd_);
        close(epoll_fd_);
    }

    void AddFd(int fd, uint32_t events) const {
        epoll_event ev = {};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void ModFd(int fd, uint32_t events) const {
        epoll_event ev = {};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void DelFd(int fd) const {
        epoll_event ev = {};
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    int Wait(int timeoutMs = -1) {
        int res = epoll_wait(epoll_fd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
        if (res < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        return res;
    }

    int GetEventFd(size_t i) const {
        return events_[i].data.fd;
    }

    uint32_t GetEvents(size_t i) const {
        return events_[i].events;
    }
private:
    int epoll_fd_ = -1;
    std::vector<struct epoll_event> events_;
};
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <system_error>
#include <sys/epoll.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024): epollFd_(epoll_create(512)), events_(maxEvent) {
        assert(epollFd_ >= 0 && events_.size() > 0);
    }
    ~Epoller() {
        close(epollFd_);
    }

    void addFd(int fd, uint32_t events) {
        epoll_event ev = {};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void modFd(int fd, uint32_t events) {
        epoll_event ev = {};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void delFd(int fd) {
        epoll_event ev = {};
        if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    int wait(int timeoutMs = -1) {
        int res = epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
        if (res < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        return res;
    }

    int getEventFd(size_t i) const {
        return events_[i].data.fd;
    }

    uint32_t getEvents(size_t i) const {
        return events_[i].events;
    }
private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};
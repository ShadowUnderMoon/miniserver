#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <sys/epoll.h>
#include <system_error>
#include <unistd.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024): events(maxEvent) {
        epollFd = epoll_create1(0);
        if (epollFd < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }
    ~Epoller() {
        close(epollFd);
    }

    void addFd(int fd, uint32_t events) const {
        epoll_event ev = {};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void modFd(int fd, uint32_t events) const {
        epoll_event ev = {};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void delFd(int fd) const {
        epoll_event ev = {};
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    int wait(int timeoutMs = -1) {
        int res = epoll_wait(epollFd, events.data(), static_cast<int>(events.size()), timeoutMs);
        if (res < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        return res;
    }

    int getEventFd(size_t i) const {
        return events[i].data.fd;
    }

    uint32_t getEvents(size_t i) const {
        return events[i].events;
    }
private:
    int epollFd = -1;
    std::vector<struct epoll_event> events;
};
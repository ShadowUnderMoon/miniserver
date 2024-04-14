#pragma once

#include <netinet/in.h>
#include <socket_descriptor.h>

#include <buffer.h>

class httpConn {

public:
    httpConn(ServerSocket&& sock, sockaddr_in addr) : sock_(std::move(sock)), addr_(addr) {
        
    }
    inline static std::atomic<int> userCount;
private:
    ServerSocket sock_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;
    Buffer writeBuff_;
};
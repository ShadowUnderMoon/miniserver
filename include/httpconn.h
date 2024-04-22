#pragma once

#include <netinet/in.h>
#include <socket_descriptor.h>

#include <buffer.h>

class httpConn {

public:
    httpConn() {}
    httpConn(std::unique_ptr<ServerSocket> sock, sockaddr_in addr) : sock(std::move(sock)), addr(addr) {
        
    }
    inline static std::atomic<int> userCount;
private:
    std::unique_ptr<ServerSocket> sock;
    struct sockaddr_in addr;

    bool isClose;

    int iovCnt;
    struct iovec iov[2];

    Buffer readBuff;
    Buffer writeBuff;
};
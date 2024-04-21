#pragma once

#include <netinet/in.h>
#include <socket_descriptor.h>

#include <buffer.h>

class httpConn {

public:
    httpConn() {}
    httpConn(ServerSocket&& sock, sockaddr_in addr) : sock(std::move(sock)), addr(addr) {
        
    }
    inline static std::atomic<int> userCount;
private:
    ServerSocket sock;
    struct sockaddr_in addr;

    bool isClose;

    int iovCnt;
    struct iovec iov[2];

    Buffer readBuff;
    Buffer writeBuff;
};
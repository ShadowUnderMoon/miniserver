#pragma once

#include <netinet/in.h>
#include <socket_descriptor.h>
class httpConn {

private:
    ServerSocket sock_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

};
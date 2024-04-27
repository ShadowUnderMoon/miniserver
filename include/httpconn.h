#pragma once

#include <cstddef>
#include <netinet/in.h>
#include <socket_descriptor.h>
#include <spdlog/spdlog.h>
#include <buffer.h>

class HttpConn {

public:
    HttpConn() {}
    HttpConn(std::unique_ptr<ServerSocket> sock, sockaddr_in addr) : sock_(std::move(sock)), addr_(addr) {
        
    }
    inline static std::atomic<int> userCount;

    ssize_t Read() {
        return read_buff_.ReadFd(sock_->get());
    }

    size_t ToWriteBytes() {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    size_t Write() {
        size_t len = -1;
        do {
            if (iov_[0].iov_len + iov_[1].iov_len == 0) {
                break;
            }
            len = writev(sock_->get(), iov_, iov_cnt_);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                throw std::system_error(errno,std::generic_category());
            }
            if (len > iov_[0].iov_len) {
                iov_[1].iov_base = (uint8_t*)iov_[1].iov_base+  (len - iov_[0].iov_len);
                iov_[1].iov_len -= (len - iov_[0].iov_len);
                if (iov_[0].iov_len) {
                    write_buff_.RetrieveAll();
                    iov_[0].iov_len = 0;
                }
            } else {
                iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
                iov_[0].iov_len -= len;
                write_buff_.Retrieve(len);
            }
        } while (isET || ToWriteBytes() > 10240);

        return len;
    }
    bool Process() {
        if (read_buff_.ReadableBytes() == 0) {
            return false;
        }
        std::string read_data = read_buff_.RetrieveAllToStr();
        write_buff_.Append(read_data);    
        iov_[0].iov_base = write_buff_.Peak();
        iov_[0].iov_len = write_buff_.ReadableBytes();
        iov_cnt_ = 1;
        return true;
    }

    int GetFd() {
        return sock_->get();
    }
public:
    inline static bool isET = true;
private:
    std::unique_ptr<ServerSocket> sock_;
    struct sockaddr_in addr_;

    bool is_close_{};

    int iov_cnt_{};
    struct iovec iov_[2]{};

    Buffer read_buff_;
    Buffer write_buff_;
};
#pragma once

#include "httprequest.h"
#include "utils.h"
#include <cstddef>
#include <netinet/in.h>
#include <socket_descriptor.h>
#include <spdlog/spdlog.h>
#include <buffer.h>
#include <httpresponse.h>
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
                iov_[1].iov_base = (uint8_t*)iov_[1].iov_base +  (len - iov_[0].iov_len);
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

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    bool Process() {
        request_.Init();
        if (read_buff_.ReadableBytes() == 0) {
            return false;
        }
        if (request_.Parse(read_buff_)) {
            SPDLOG_LOGGER_INFO(spdlog::get("miniserver"), request_.path());
            response_.Init(src_dir, request_.path(), request_.IsKeepAlive(), 200);
        } else {
            response_.Init(src_dir, request_.path(), false, 400);
        }

        response_.MakeResponse(write_buff_);
        // response header
        iov_[0].iov_base = const_cast<char*>(write_buff_.Peak());
        iov_[0].iov_len = write_buff_.ReadableBytes();
        iov_cnt_ = 1;
        // response body
        if (response_.FileLen() > 0 && response_.File()) {
            iov_[1].iov_base = response_.File();
            iov_[1].iov_len = response_.FileLen();
            iov_cnt_ = 2;
        }
        SPDLOG_LOGGER_INFO(spdlog::get("miniserver"), "filesize: {}, iovCnt: {}, Total: {} Bytes", response_.FileLen(), iov_cnt_, ToWriteBytes());
        return true;
    }

    bool Echo() {
        if (read_buff_.ReadableBytes() == 0) {
            return false;
        }
        std::string read_data = read_buff_.RetrieveAllToStr();
        SPDLOG_LOGGER_INFO(spdlog::get("miniserver"), EscapeString(read_data));
        write_buff_.Append(read_data);    
        iov_[0].iov_base = const_cast<char*>(write_buff_.Peak());
        iov_[0].iov_len = write_buff_.ReadableBytes();
        iov_cnt_ = 1;
        return true;
    }
    int GetFd() {
        return sock_->get();
    }
public:
    inline static bool isET = true;
    inline static std::string src_dir;
    inline static std::atomic<int> user_count = 0;
private:
    std::unique_ptr<ServerSocket> sock_;
    struct sockaddr_in addr_;

    bool is_close_{};

    int iov_cnt_{};
    struct iovec iov_[2]{};

    Buffer read_buff_;
    Buffer write_buff_;
    
    HttpRequest request_;
    HttpResponse response_;
};
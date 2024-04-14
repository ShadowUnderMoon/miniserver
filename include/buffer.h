#pragma once

#include <bits/types/struct_iovec.h>
#include <cerrno>
#include <cstddef>
#include <sys/types.h>
#include <system_error>
#include <vector>
#include <sys/uio.h>
class Buffer {
public:
    static const size_t initialSize = 1024;

    explicit Buffer(size_t initialSize_ = initialSize)
        : buffer_(initialSize_), readerIndex_(0), writerIndex_(0) {

    }

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const {
        return readerIndex_;
    }

    size_t availableBytes() const {
        return prependableBytes() + writableBytes();
    }

    void append(const char* str, size_t len) {
        if (writableBytes() < len) {
            if (writableBytes() + prependableBytes() < len) {
                buffer_.resize(readableBytes() + len);
            } else {
                size_t readble = readableBytes();
                std::copy(buffer_.data() + readerIndex_, buffer_.data() + writerIndex_, buffer_.data());
                readerIndex_ = 0;
                writerIndex_ = readble;
            }
        }
        std::copy(str, str + len, buffer_.data() + writerIndex_);
        writerIndex_ += len;
    }

    ssize_t readFd(int fd) {
        char extrabuff[65536];
        struct iovec iov[2];
        ssize_t total = 0;
        while (true) {
            size_t writable = writableBytes();
            iov[0].iov_base = buffer_.data() + writerIndex_;
            iov[0].iov_len = writable;
            iov[1].iov_base = extrabuff;
            iov[1].iov_len = sizeof(extrabuff);

            ssize_t len = readv(fd, iov, 2);

            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    throw std::system_error(errno,std::generic_category());
                }
            } else {
                if (static_cast<size_t>(len) <= writable) {
                    writerIndex_ += len;
                } else {
                    writerIndex_ = buffer_.size();
                    append(extrabuff, len - writable);
                }
            }
            total += len;
        }
        return total;
    }
private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
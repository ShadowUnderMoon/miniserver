#pragma once

#include <bits/types/struct_iovec.h>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <sys/types.h>
#include <system_error>
#include <vector>
#include <sys/uio.h>
class Buffer {
public:
    static const size_t initialSize = 1024;

    explicit Buffer(size_t initial_size = initialSize)
        : buffer_(initial_size) {

    }

    size_t ReadableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    const char* Peak() {
        return buffer_.data() + readerIndex_;
    }

    const char* BeginWrite() {
        return buffer_.data() + writerIndex_;
    }
    size_t prependableBytes() const {
        return readerIndex_;
    }

    void Retrieve(size_t len) {
        readerIndex_ += len;
    }

    void RetrieveUntil(const char* end) {
        Retrieve(end - Peak());
    }
    void RetrieveAll() {
        readerIndex_ = 0;
        writerIndex_ = 0;
    }
    std::string RetrieveAllToStr() {
        std::string str(Peak(), ReadableBytes());
        RetrieveAll();
        return str;
    }
    size_t availableBytes() const {
        return prependableBytes() + writableBytes();
    }

    void Append(const std::string& str) {
        Append(str.data(), str.size());
    }

    void Append(const char* str, size_t len) {
        if (writableBytes() < len) {
            if (writableBytes() + prependableBytes() < len) {
                buffer_.resize(ReadableBytes() + len);
            } else {
                size_t readble = ReadableBytes();
                std::copy(buffer_.data() + readerIndex_, buffer_.data() + writerIndex_, buffer_.data());
                readerIndex_ = 0;
                writerIndex_ = readble;
            }
        }
        std::copy(str, str + len, buffer_.data() + writerIndex_);
        writerIndex_ += len;
    }

    ssize_t ReadFd(int fd) {
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
                }
                throw std::system_error(errno,std::generic_category());
            }

            if (static_cast<size_t>(len) <= writable) {
                writerIndex_ += len;
            } else {
                writerIndex_ = buffer_.size();
                Append(extrabuff, len - writable);
            }
            total += len;
        }
        return total;
    }
// private:
    std::vector<char> buffer_;
    size_t readerIndex_{};
    size_t writerIndex_{};
};
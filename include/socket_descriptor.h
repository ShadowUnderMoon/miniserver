#pragma once
#include <asm-generic/socket.h>
#include <spdlog/spdlog.h>
#include <system_error>
#include <unistd.h> // For close()
#include <stdexcept> // For std::runtime_error
#include <sys/socket.h> // For socket, bind, listen, accept
#include <netinet/in.h> // For sockaddr_in
#include <fcntl.h>

class ServerSocket {
private:
    int sockfd_ = -1;

public:
    ServerSocket() {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    ServerSocket(int sockfd): sockfd_(sockfd){
    }

    ServerSocket(ServerSocket&& other) noexcept : sockfd_(other.sockfd_) {
        other.sockfd_ = -1;
    }

    ServerSocket& operator=(ServerSocket&& other) noexcept {
        if (this != &other) {
            close();
            sockfd_ = other.sockfd_;
            other.sockfd_ = -1;
        }
        return *this;
    }

    ~ServerSocket() {
        close();
    }

    void close() noexcept {
        if (sockfd_ != -1) {
            try {
                int res = ::close(sockfd_);
                if (res < 0) {
                    throw std::system_error(errno, std::generic_category());
                }
            }
            catch (const std::system_error& ex) {
                spdlog::get("miniserver")->error("{}, {}", ex.code().value(), ex.what());
            }
            sockfd_ = -1;
        }
    }

    int get() const {
        return sockfd_;
    }

    bool isValid() const {
        return sockfd_ != -1;
    }

    void listenTo(int port, int max_established_sock) {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        setLingerOption(2);
        setNonblock();
        int optval = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        if (bind(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        if (listen(sockfd_, max_established_sock) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    static ServerSocket acceptConnection(const ServerSocket& listenfd) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSockfd = accept(listenfd.get(), (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSockfd < 0) {
             throw std::system_error(errno, std::generic_category());
        }
        return std::move(ServerSocket(clientSockfd));
    }

    void setLingerOption(int lingerTime) {
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = lingerTime;

        if (setsockopt(sockfd_, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void setNonblock() {
        if (fcntl(sockfd_, F_SETFL, fcntl(sockfd_, F_GETFD, 0) | O_NONBLOCK) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    ServerSocket(const ServerSocket&) = delete;
    ServerSocket& operator=(const ServerSocket&) = delete;
};
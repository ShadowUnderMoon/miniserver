#pragma once
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h> // For sockaddr_in
#include <spdlog/spdlog.h>
#include <sys/socket.h> // For socket, bind, listen, accept
#include <system_error>
#include <unistd.h> // For close()

class ServerSocket {
private:
    int sockfd = -1;

public:
    ServerSocket(): sockfd(get_new_scoket()){
    }

    explicit ServerSocket(int sockfd): sockfd(sockfd){
    }

    static int get_new_scoket() {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        return fd;
    }

    ServerSocket(ServerSocket&& other) noexcept : sockfd(other.sockfd) {
        other.sockfd = -1;
    }

    ServerSocket& operator=(ServerSocket&& other) noexcept {
        if (this != &other) {
            close();
            sockfd = other.sockfd;
            other.sockfd = -1;
        }
        return *this;
    }

    ~ServerSocket() {
        spdlog::get("miniserver")->info("close the socket");
        close();
    }

    void close() noexcept {
        if (sockfd != -1) {
            try {
                int res = ::close(sockfd);
                if (res < 0) {
                    throw std::system_error(errno, std::generic_category());
                }
            }
            catch (const std::system_error& ex) {
                spdlog::get("miniserver")->error("{}, {}", ex.code().value(), ex.what());
            }
            sockfd = -1;
        }
    }

    int get() const {
        return sockfd;
    }

    bool isValid() const {
        return sockfd != -1;
    }

    void listenTo(int port, int max_established_sock) const {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        setLingerOption(2);
        setNonblock();
        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        if (listen(sockfd, max_established_sock) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }



    void setLingerOption(int lingerTime) const {
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = lingerTime;

        if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void setNonblock() const {
        if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) < 0) {
            throw std::system_error(errno, std::generic_category());
        }
    }

    ServerSocket(const ServerSocket&) = delete;
    ServerSocket& operator=(const ServerSocket&) = delete;
};
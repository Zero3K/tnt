#include "tcp_connection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <netinet/tcp.h>


TcpConnection::TcpConnection(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout) :
    ip_(ip), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {}

TcpConnection::~TcpConnection() {
    Terminate();
}

void TcpConnection::EstablishConnection() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error(
            "TCP error while using socket(): " + std::string(strerror(errno))
        );
    }

    timeval tvRead = {};
    tvRead.tv_sec = static_cast<int>(readTimeout_.count()) / 1000;
    tvRead.tv_usec = static_cast<int>(readTimeout_.count()) % 1000 * 1000;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tvRead, sizeof(tvRead));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tvRead, sizeof(tvRead));

    char flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    int buf = 131072 * 32;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf, sizeof(buf));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));

    sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port = htons(static_cast<uint16_t>(port_)),
        .sin_addr = in_addr { inet_addr(ip_.c_str()) }
    };

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    timeval tvConnect = {};
    tvConnect.tv_sec = static_cast<int>(connectTimeout_.count()) / 1000;
    tvConnect.tv_usec = static_cast<int>(connectTimeout_.count()) % 1000 * 1000;

    int flags = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        if (errno == EINPROGRESS) {
            if (select(sock + 1, nullptr, &fds, nullptr, &tvConnect) != 1) {
                close(sock);
                throw std::runtime_error(
                    "TCP error while using select(): " + std::string(strerror(errno))
                );
            }

            int sockErrCode;
            socklen_t optLen = sizeof(sockErrCode);

            getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErrCode, &optLen);

            if (sockErrCode != 0) {
                close(sock);
                throw std::runtime_error(
                    "TCP socket error: " + std::string(strerror(errno))
                );
            }
        } else {
            close(sock);
            throw std::runtime_error(
                "TCP error while using connect(): " + std::string(strerror(errno))
            );
        }
    }

    fcntl(sock, F_SETFL, flags & (~O_NONBLOCK));

    sock_ = sock;
}

void TcpConnection::SendData(const std::string& data) {
    if (!IsRunning()) {
        throw std::runtime_error("Can't send until connection is established.");
    }

    std::unique_lock lock(sendMtx_);

    if (send(sock_, &data[0], data.size(), 0) == -1) {
        Terminate();
        throw std::runtime_error(
            "TCP error while using send(): " + std::string(strerror(errno))
        );
    }
}

std::string TcpConnection::ReceiveData(size_t bufferSize) {
    if (!IsRunning()) {
        throw std::runtime_error("Can't receive until connection is established.");
    }

    std::unique_lock lock(rcvMtx_);

    std::string result;
    if (bufferSize == 0) {
        result.resize(4);

        int sz = recv(sock_, &result[0], 4, MSG_WAITALL);
        if (sz < 4) {
            Terminate();
            throw std::runtime_error(
                "TCP error while using recv(): " + std::string(strerror(errno))
            );
        }
        
        bufferSize = ntohl(*reinterpret_cast<uint32_t*>(&result[0]));

        if (bufferSize == 0)
            return result;
    }

    std::string buf;
    buf.resize(bufferSize);

    if (recv(sock_, &buf[0], bufferSize, MSG_WAITALL) < bufferSize) {
        Terminate();
        throw std::runtime_error(
            "TCP error while using recv(): " + std::string(strerror(errno))
        );
    }

    result += buf;

    return result;
}

void TcpConnection::Terminate() {
    if (sock_ != -1) {
        shutdown(sock_, SHUT_RDWR);
        close(sock_);
        sock_ = -1;
    }
}

bool TcpConnection::IsRunning() const {
    return sock_ != -1;
}
#include "tcp_connection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>


TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout) :
    ip_(ip), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {}

TcpConnect::~TcpConnect() {
    CloseConnection();
}

void TcpConnect::EstablishConnection() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        throw std::runtime_error(strerror(errno));
    }

    timeval tvRead = {};
    tvRead.tv_sec = static_cast<int>(readTimeout_.count()) / 1000;
    tvRead.tv_usec = static_cast<int>(readTimeout_.count()) % 1000 * 1000;

    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tvRead, sizeof(tvRead));
    setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &tvRead, sizeof(tvRead));

    sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port = htons(static_cast<uint16_t>(port_)),
        .sin_addr = in_addr { inet_addr(ip_.c_str()) }
    };

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock_, &fds);

    timeval tvConnect = {};
    tvConnect.tv_sec = static_cast<int>(connectTimeout_.count()) / 1000;
    tvConnect.tv_usec = static_cast<int>(connectTimeout_.count()) % 1000 * 1000;

    int flags = fcntl(sock_, F_GETFL);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    if (connect(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        if (errno == EINPROGRESS) {
            if (select(sock_ + 1, nullptr, &fds, nullptr, &tvConnect) != 1) {
                throw std::runtime_error(std::string("select failure: ") + strerror(errno));
            }

            int sockErrc;
            socklen_t optLen = sizeof(sockErrc);

            getsockopt(sock_, SOL_SOCKET, SO_ERROR, &sockErrc, &optLen);

            if (sockErrc != 0) {
                throw std::runtime_error(std::string("sock failure: ") + strerror(sockErrc));
            }
        } else {
            throw std::runtime_error(std::string("connect failed: ") + strerror(errno));
        }
    }

    fcntl(sock_, F_SETFL, flags & (~O_NONBLOCK));
}

void TcpConnect::SendData(const std::string& data) const {
    if (send(sock_, &data[0], data.size(), 0) == -1) {
        throw std::runtime_error(std::string("send failure: ") + strerror(errno));
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    std::string result;

    if (bufferSize == 0) {
        result.resize(4);

        if (recv(sock_, &result[0], 4, 0) < 4)
            throw std::runtime_error(std::string("recv failure whiel getting str len"));
        bufferSize = ntohl(*reinterpret_cast<uint32_t*>(&result[0]));

        if (bufferSize == 0)
            return result;
    }

    std::string buf;
    buf.resize(bufferSize);

    int total = 0;
    while (total < bufferSize) {
        int res = recv(sock_, &buf[total], bufferSize - total, 0);
        if (res == -1 || res == 0) {
            throw std::runtime_error(std::string("recv failure: ") + strerror(errno));
        }
        total += res;
    }

    result += buf;

    return result;
}

void TcpConnect::CloseConnection() {
    close(sock_);
}

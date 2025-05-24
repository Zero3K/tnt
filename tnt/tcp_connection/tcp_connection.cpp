#include "tcp_connection.h"
#include "exception.h"
#include <sys/socket.h>
#include <arpa/inet.h>
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
    CloseConnection();
}

TcpConnection::TcpConnection(TcpConnection&& other) : ip_(other.ip_), port_(other.port_), 
        connectTimeout_(other.connectTimeout_), readTimeout_(other.readTimeout_)  {
    
}

TcpConnection::TcpConnection(const TcpConnection& other) : ip_(other.ip_), port_(other.port_), 
        connectTimeout_(other.connectTimeout_), readTimeout_(other.readTimeout_) {
    
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) {
    std::swap(ip_, other.ip_);
    std::swap(port_, other.port_);
    std::swap(connectTimeout_, other.connectTimeout_);
    std::swap(readTimeout_, other.readTimeout_);
    std::swap(sock_, other.sock_);

    return *this;
}

TcpConnection& TcpConnection::operator=(const TcpConnection& other) {
    ip_ = other.ip_;
    port_ = other.port_;
    connectTimeout_ = other.connectTimeout_;
    readTimeout_ = other.readTimeout_;
    sock_ = -1;

    return *this;
}

void TcpConnection::EstablishConnection() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        throw std::runtime_error(strerror(errno));
    }

    timeval tvRead = {};
    tvRead.tv_sec = static_cast<int>(readTimeout_.count()) / 1000;
    tvRead.tv_usec = static_cast<int>(readTimeout_.count()) % 1000 * 1000;

    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tvRead, sizeof(tvRead));
    setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &tvRead, sizeof(tvRead));

    char flag = 1;
    setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    int buf = 131072 * 32;
    setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, &buf, sizeof(buf));
    setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));

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

void TcpConnection::SendData(const std::string& data) const {
    if (send(sock_, &data[0], data.size(), 0) == -1) {
        throw std::runtime_error(std::string("send failure: ") + strerror(errno));
    }
}

std::string TcpConnection::ReceiveData(size_t bufferSize) const {
    std::string result;

    if (bufferSize == 0) {
        result.resize(4);

        int sz = recv(sock_, &result[0], 4, MSG_WAITALL);
        if (sz < 4)
            throw TcpTimeoutError();
        bufferSize = ntohl(*reinterpret_cast<uint32_t*>(&result[0]));

        if (bufferSize == 0)
            return result;
    }

    std::string buf;
    buf.resize(bufferSize);

    if (recv(sock_, &buf[0], bufferSize, MSG_WAITALL) < bufferSize) {
        throw std::runtime_error(std::string("recv failure: ") + strerror(errno));
    }

    result += buf;

    return result;
}

void TcpConnection::CloseConnection() {
    shutdown(sock_, SHUT_RDWR);
}

#pragma once

#include <string>
#include <chrono>

using namespace std::chrono_literals;

class TcpConnection {
public:
    TcpConnection(std::string ip, int port, std::chrono::milliseconds connectTimeout = 1000ms, std::chrono::milliseconds readTimeout = 1000ms);
    ~TcpConnection();

    // Establish connection.
    void EstablishConnection();

    // Send data to remote host.
    void SendData(const std::string& data) const;

    // Recieve data from remote host.
    std::string ReceiveData(size_t bufferSize = 0) const;

    // Close connection.
    void CloseConnection();
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_;
};

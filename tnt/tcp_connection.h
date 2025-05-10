#pragma once

#include <string>
#include <chrono>


class TcpConnection {
public:
    TcpConnection(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout);
    
    ~TcpConnection();

    void EstablishConnection();

    void SendData(const std::string& data) const;

    std::string ReceiveData(size_t bufferSize = 0) const;

    void CloseConnection();
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_;
};

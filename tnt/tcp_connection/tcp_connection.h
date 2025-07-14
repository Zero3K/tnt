#pragma once

#include <string>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

class TcpConnection {
public:
    TcpConnection(std::string ip, int port, std::chrono::milliseconds connectTimeout = 10000ms, std::chrono::milliseconds readTimeout = 10000ms);

    ~TcpConnection();

    // Establish connection.
    void EstablishConnection();

    // Send data to remote host.
    void SendData(const std::string& data);

    // Recieve data from remote host.
    std::string ReceiveData(size_t bufferSize = 0);

    // Close connection.
    void Terminate();
    
    // Returns true if remote host is connected and false otherwise.
    bool IsRunning() const;
private:
    std::string ip_;
    int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;

    int sock_ = -1;
};

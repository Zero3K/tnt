#pragma once

#include <stdexcept>
#include <string>


class TcpError : public std::runtime_error {
public:
    explicit TcpError(std::string message) : std::runtime_error("Encountered TCP error: " + message) {}
};

class TcpTimeoutError : public TcpError {
public:
    TcpTimeoutError() : TcpError("Timeout") {}
};
#pragma once

#include <cstdint>
#include <string>


struct Message {
    enum class Id : uint8_t {
        Choke = 0,
        Unchoke,
        Interested,
        NotInterested,
        Have,
        BitField,
        Request,
        Piece,
        Cancel,
        Port,
        KeepAlive,
    } id;

    size_t messageLength;
    std::string payload;

    static Message Parse(const std::string& messageString);
    static Message Init(Id id, const std::string& payload = "");

    static Message InitRequest(size_t pieceIndex, size_t blockOffset, size_t blockLength);

    std::string ToString() const;
};
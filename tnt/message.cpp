#include <string>
#include "message.h"
#include <netinet/in.h>


Message Message::Parse(const std::string& messageString) {
    Message result = {};
    if (messageString.size() > 4) {
        result.messageLength = ntohl(*reinterpret_cast<const uint32_t*>(&messageString[0]));
        result.id = Message::Id(messageString[4]);
        result.payload = messageString.substr(5);
    } else result.id = Message::Id::KeepAlive;

    return result;
}

Message Message::Init(Message::Id id, const std::string& payload) {
    return Message {
        id,
        payload.size() + 1,
        payload
    };
}

std::string Message::ToString() const {
    std::string result;

    if (id != Message::Id::KeepAlive) {
        result.reserve(messageLength + 1);
        result.resize(5);

        *reinterpret_cast<int*>(&result[0]) = htonl(messageLength);
        result[4] = static_cast<int>(id);
        result += payload;
    } else {
        result.resize(4);
        result[0] = 0;
        result[1] = 0;
        result[2] = 0;
        result[3] = 0;
    }

    return result;
}
#include <string>
#include "message.h"
#include <netinet/in.h>

Message Message::Parse(const std::string& messageString) {
    Message result = {};
    if (messageString.size() > 4) {
        result.messageLength = ntohl(*reinterpret_cast<const uint32_t*>(&messageString[0]));
        result.id = MessageId(messageString[4]);
        result.payload = messageString.substr(5);
    } else result.id = MessageId::KeepAlive;

    return result;
}

/*
* Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
*/
Message Message::Init(MessageId id, const std::string& payload) {
    return Message {
        id,
        payload.size() + 1,
        payload
    };
}

/*
* Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
* Получается строка вида "<1 + payload length><message id><payload>"
* Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
* id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
*/
std::string Message::ToString() const {
    std::string result;

    if (id != MessageId::KeepAlive) {
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
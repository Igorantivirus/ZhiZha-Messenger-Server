#pragma once

#include <ctime>
#include <string>

#include <Protocol/Common/Types.hpp>
#include <Protocol/Common/Parsing.hpp>

namespace protocol::data
{

// Сообщение в комнате
struct Message
{
    MessageId id;          // id сообщения
    RoomId roomId;         // id комнаты
    UserId fromUserId;     // id отправителя
    std::string text;      // сообщение
    std::time_t createdAt; // время отправки
};
PROTOCOL_JSON_SEREALIZE(Message)

} // namespace protocol::data
#pragma once

#include <unordered_map>
#include <vector>

#include <ProtocolV1/Common/Types.hpp>
#include <ProtocolV1/Common/Parsing.hpp>
#include <ProtocolV1/Data/Messages.hpp>
#include <ProtocolV1/Data/Users.hpp>

namespace protocol::dto
{

// Отправка сообщений из комнаты
struct GetMessagesResponseDto
{
    RoomId roomId;
    std::vector<data::Message> messages;
    bool hasMore;
    std::unordered_map<UserId, data::UserDisplayInfo> senders;
};
PROTOCOL_JSON_SEREALIZE(GetMessagesResponseDto)

} // namespace protocol::dto
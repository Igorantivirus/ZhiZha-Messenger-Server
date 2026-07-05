#pragma once

#include <unordered_map>
#include <vector>

#include <Protocol/Common/Types.hpp>
#include <Protocol/Common/Parsing.hpp>
#include <Protocol/Data/Messages.hpp>
#include <Protocol/Data/Users.hpp>

namespace protocol::dto
{

// Отправка сообщений из комнаты
struct GetMessagesResponseDto
{
    RoomId roomId;
    std::vector<data::Message> messages;
    std::unordered_map<UserId, data::UserDisplayInfo> senders;
    bool hasMore;
};
PROTOCOL_JSON_SEREALIZE(GetMessagesResponseDto)

} // namespace protocol::dto
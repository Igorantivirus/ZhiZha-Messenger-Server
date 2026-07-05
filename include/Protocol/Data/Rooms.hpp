#pragma once

#include <cstdint>
#include <string>

#include <Protocol/Common/Parsing.hpp>
#include <Protocol/Common/Types.hpp>
#include <Protocol/Data/Messages.hpp>
#include <Protocol/Data/Users.hpp>

namespace protocol::data
{

// Поведенческие настройки комнаты (без идентичности)
struct RoomInfo
{
    RoomKind kind;
    JoinPolicy joinPolicy;
    WritePolicy writePolicy;
};
PROTOCOL_JSON_SEREALIZE(RoomInfo)

// Публичная карточка комнаты.
struct Room
{
    RoomId id;
    std::string name;
    RoomInfo info;
};
PROTOCOL_JSON_SEREALIZE(Room)

// Комната в контексте конкретного пользователя
struct RoomWithLastMessage
{
    Room roomInfo;
    Message lastMessage;
    MemberRole senderRole;
    std::uint32_t participantsCount;
};
PROTOCOL_JSON_SEREALIZE(RoomWithLastMessage)

// Участник комнаты
struct Member
{
    UserId userId;
    UserDisplayInfo display;
    MemberRole role;
};
PROTOCOL_JSON_SEREALIZE(Member)

} // namespace protocol::data
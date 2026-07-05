#pragma once

#include <Utils/Types.hpp>

#include "Message.hpp"
#include "Room.hpp"

namespace chat
{

// View-структура для UI: комната + последнее сообщение + роль вызывающего
// + число участников (берётся динамически из members при формировании).
struct RoomForUser
{
    Room room;
    Message lastMessage;
    utils::MemberRole role;
    unsigned participantsCount;
};

} // namespace chat

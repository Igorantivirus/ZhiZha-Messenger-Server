#pragma once

#include <cstdint>

#include <Protocol/Rooms.hpp>

#include "Message.hpp"
#include "Room.hpp"

struct RoomWithLastMessage
{
    Room room;
    Message msg;
    std::uint32_t participantsCount;
    protocol::rooms::MemberRole role; // роль запрашивающего пользователя в этой комнате
};
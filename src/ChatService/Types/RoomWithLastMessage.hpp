#pragma once

#include <cstdint>

#include "Message.hpp"
#include "Room.hpp"

struct RoomWithLastMessage
{
    Room room;
    Message msg;
    std::uint32_t participantsCount;
};
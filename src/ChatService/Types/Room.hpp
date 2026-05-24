#pragma once

#include <ctime>
#include <string>

#include <Protocol/Rooms.hpp>
#include <Protocol/Types.hpp>

struct Room
{
    protocol::RoomId id;
    std::string name;
    protocol::rooms::RoomInfo info;
    std::time_t createdAt;
};
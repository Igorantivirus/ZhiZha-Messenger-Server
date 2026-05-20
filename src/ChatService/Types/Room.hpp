#pragma once

#include <string>
#include <ctime>

#include <Utils/Types.hpp>

#include "RoomInfo.hpp"

struct Room
{
    RoomId id;
    std::string name;
    info::RoomInfo info;
    std::time_t createdAt;
};
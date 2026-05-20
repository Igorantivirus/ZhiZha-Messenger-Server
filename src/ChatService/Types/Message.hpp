#pragma once

#include <string>
#include <ctime>

#include <Utils/Types.hpp>

struct Message
{
    MessageId id;
    RoomId roomId;
    UserId fromUserId;
    std::string text;
    std::time_t createdAt;
};
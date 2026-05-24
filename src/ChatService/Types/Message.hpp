#pragma once

#include <ctime>
#include <string>

#include <Protocol/Types.hpp>

struct Message
{
    protocol::MessageId id;
    protocol::RoomId roomId;
    protocol::UserId fromUserId;
    std::string text;
    std::time_t createdAt;
};
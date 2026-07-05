#pragma once

#include <ctime>
#include <string>

#include <Utils/Types.hpp>

namespace chat
{

struct Message
{
    utils::MessageId id;
    utils::RoomId roomId;
    utils::UserId fromUserId;
    std::string text;
    std::time_t createdAt;
};

} // namespace chat
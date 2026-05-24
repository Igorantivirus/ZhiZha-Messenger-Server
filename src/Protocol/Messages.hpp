#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "Types.hpp"

namespace protocol::messages
{
struct Message
{
    MessageId id;
    UserId fromUserId;
    std::string text;
    std::time_t createdAt;
};

struct MessagesResponse
{
    RoomId roomId;
    std::vector<Message> messages;
    bool hasMore;
};

} // namespace protocol::messages
#pragma once

#include <ctime>

#include <Utils/Types.hpp>

namespace chat
{

struct Member
{
    utils::RoomId roomId;
    utils::UserId userId;
    utils::MemberRole role;
    utils::MessageId lastReadMessageId;
    std::time_t joinedAt;
};

} // namespace chat
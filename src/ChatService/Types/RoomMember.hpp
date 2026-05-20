#pragma once

#include <ctime>

#include <Utils/Types.hpp>

#include "MemberRole.hpp"

struct RoomMember
{
    RoomId roomId;
    UserId userId;
    std::time_t joinedAt;
    MemberRole role;
    MessageId lastReadMessageId;
};
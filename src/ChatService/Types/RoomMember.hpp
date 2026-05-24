#pragma once

#include <ctime>

#include <Protocol/Types.hpp>

#include "MemberRole.hpp"

struct RoomMember
{
    protocol::RoomId roomId;
    protocol::UserId userId;
    std::time_t joinedAt;
    MemberRole role;
    protocol::MessageId lastReadMessageId;
};
#pragma once

#include <ctime>

#include <Protocol/Rooms.hpp>
#include <Protocol/Types.hpp>

struct RoomMember
{
    protocol::RoomId roomId;
    protocol::UserId userId;
    std::time_t joinedAt;
    protocol::rooms::MemberRole role;
    protocol::MessageId lastReadMessageId;
};
#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include <Protocol/Rooms.hpp>
#include <Protocol/Types.hpp>

#include <ChatService/Types/Room.hpp>

class IRoomRepository
{
public:
    virtual ~IRoomRepository() = default;
    virtual protocol::RoomId create(const std::string &name, const protocol::rooms::RoomInfo info, const std::time_t createdAt) = 0;
    virtual std::optional<Room> findById(protocol::RoomId id) const = 0;
    virtual std::vector<Room> findForUser(const protocol::UserId userId) const = 0;
    virtual void remove(const protocol::RoomId id) = 0;
};
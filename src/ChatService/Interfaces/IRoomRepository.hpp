#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include <Utils/Types.hpp>

#include <ChatService/Types/Room.hpp>
#include <ChatService/Types/RoomInfo.hpp>

class IRoomRepository
{
public:
    virtual ~IRoomRepository() = default;
    virtual RoomId create(const std::string &name, const info::RoomInfo info, const std::time_t createdAt) = 0;
    virtual std::optional<Room> findById(RoomId id) const = 0;
    virtual std::vector<Room> findForUser(const UserId userId) const = 0;
    virtual void remove(const RoomId id) = 0;
};
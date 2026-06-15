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
    virtual std::vector<Room> findForUser(const protocol::UserId userId, const unsigned limit, std::optional<protocol::RoomId> lastLoaded) const = 0;
    virtual void remove(const protocol::RoomId id) = 0;
    virtual std::vector<Room> getRoomsByQuery(std::string query, unsigned limit) = 0;
    // Обновляет имя и политики комнаты (createdAt не трогает).
    // Возвращает true, если комната с таким id существовала и была обновлена.
    virtual bool changeRoomsInfo(const protocol::rooms::Room &newInfo) = 0;
};
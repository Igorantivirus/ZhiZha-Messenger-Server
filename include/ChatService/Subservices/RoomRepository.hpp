#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Utils/Types.hpp>

#include <ChatService/Types/Room.hpp>

namespace chat
{

// Репозиторий комнат. Знает только таблицу rooms.
// Список участников и счётчики берутся через MembersRepository.
class RoomRepository
{
public:
    explicit RoomRepository(SQLite::Database &db);

    // createdAt ставит сам репозиторий (серверное время).
    Room create(const std::string &name, const utils::RoomInfo info);

    std::optional<Room> findById(utils::RoomId id) const;

    // Пакетная выборка для второго шага после получения ids из MembersRepository.
    // Один запрос с WHERE id IN (?, ?, ...). Для пустого ids возвращает {}.
    std::vector<Room> findByIds(std::span<const utils::RoomId> ids) const;

    void remove(const utils::RoomId id);

    void updateRoom(const utils::RoomId id, const std::string &name, const utils::RoomInfo info);

    std::vector<Room> findByQuery(const std::string &query, const unsigned limit) const;

private:
    SQLite::Database &db_;
};

} // namespace chat

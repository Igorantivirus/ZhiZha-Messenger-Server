#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <memory>
#include <string_view>

#include <ChatService/Interfaces/IRoomRepository.hpp>

class SQLiteRoomRepository : public IRoomRepository
{
public:
    explicit SQLiteRoomRepository(std::shared_ptr<SQLite::Database> db)
        : db_(std::move(db))
    {
        db_->exec(CREATE_TABLE_COMMAND.data());
    }

    RoomId create(const std::string &name,
                  const info::RoomInfo info,
                  const std::time_t createdAt) override
    {
        SQLite::Statement insert(*db_, INSERT_ROOM_COMMAND.data());
        insert.bind(1, name);
        insert.bind(2, roomKindToString(info.kind));
        insert.bind(3, joinPolicyToString(info.joinPolicy));
        insert.bind(4, writePolicyToString(info.writePolicy));
        insert.bind(5, static_cast<std::int64_t>(createdAt));
        insert.exec();
        return static_cast<RoomId>(db_->getLastInsertRowid());
    }

    std::optional<Room> findById(RoomId id) const override
    {
        SQLite::Statement query(*db_, SELECT_BY_ID.data());
        query.bind(1, static_cast<std::int64_t>(id));
        if (query.executeStep())
            return rowToRoom(query);
        return std::nullopt;
    }

    std::vector<Room> findForUser(const UserId userId) const override
    {
        // JOIN с roomMembers — это и есть причина, по которой метод тут:
        // нам нужны полные Room-объекты, а не только roomId.
        SQLite::Statement query(*db_, SELECT_FOR_USER.data());
        query.bind(1, static_cast<std::int64_t>(userId));

        std::vector<Room> result;
        while (query.executeStep())
            result.push_back(rowToRoom(query));
        return result;
    }

    void remove(const RoomId id) override
    {
        SQLite::Statement stmt(*db_, DELETE_ROOM_COMMAND.data());
        stmt.bind(1, static_cast<std::int64_t>(id));
        stmt.exec();
    }

private:
    // Конвертация одной строки результата в Room.
    // Предполагаю, что Room имеет поля: id, name, info{kind, joinPolicy, writePolicy}, createdAt.
    static Room rowToRoom(SQLite::Statement &query)
    {
        Room room;
        room.id = static_cast<RoomId>(query.getColumn(0).getInt64());
        room.name = query.getColumn(1).getString();
        room.info.kind = roomKindFromString(query.getColumn(2).getString());
        room.info.joinPolicy = joinPolicyFromString(query.getColumn(3).getString());
        room.info.writePolicy = writePolicyFromString(query.getColumn(4).getString());
        room.createdAt = static_cast<std::time_t>(query.getColumn(5).getInt64());
        return room;
    }

    // Конверсия enum'ов в строки и обратно.
    // Эти функции должны жить где-то в Types/RoomInfo.hpp или рядом —
    // здесь я предполагаю, что они доступны.
    // Если у тебя они называются иначе или enum'ы хранятся как int — поправь.
    static std::string roomKindToString(info::RoomKind k);
    static info::RoomKind roomKindFromString(const std::string &s);
    static std::string joinPolicyToString(info::JoinPolicy p);
    static info::JoinPolicy joinPolicyFromString(const std::string &s);
    static std::string writePolicyToString(info::WritePolicy p);
    static info::WritePolicy writePolicyFromString(const std::string &s);

    std::shared_ptr<SQLite::Database> db_;

    // ─── SQL ───────────────────────────────────────────────────────

    static constexpr std::string_view CREATE_TABLE_COMMAND =
        "CREATE TABLE IF NOT EXISTS rooms("
        "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name        TEXT    NOT NULL,"
        "kind        TEXT    NOT NULL CHECK (kind IN ('direct','group','channel')),"
        "joinPolicy  TEXT    NOT NULL CHECK (joinPolicy IN ('closed','byMember','byAdmin','public')),"
        "writePolicy TEXT    NOT NULL CHECK (writePolicy IN ('everyone','adminsOnly')),"
        "createdAt   INTEGER NOT NULL"
        ")";

    static constexpr std::string_view INSERT_ROOM_COMMAND =
        "INSERT INTO rooms (name, kind, joinPolicy, writePolicy, createdAt) "
        "VALUES (?, ?, ?, ?, ?)";

    static constexpr std::string_view SELECT_BY_ID =
        "SELECT id, name, kind, joinPolicy, writePolicy, createdAt "
        "FROM rooms WHERE id = ?";

    // JOIN: rooms ⋈ roomMembers, фильтр по userId.
    // Возвращает все комнаты, в которых состоит данный юзер.
    static constexpr std::string_view SELECT_FOR_USER =
        "SELECT r.id, r.name, r.kind, r.joinPolicy, r.writePolicy, r.createdAt "
        "FROM rooms r "
        "JOIN roomMembers rm ON rm.roomId = r.id "
        "WHERE rm.userId = ? "
        "ORDER BY r.id";

    static constexpr std::string_view DELETE_ROOM_COMMAND =
        "DELETE FROM rooms WHERE id = ?";
};
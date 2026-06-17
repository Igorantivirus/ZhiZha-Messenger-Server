#pragma once

#include "Protocol/Rooms.hpp"
#include "magic_enum/magic_enum.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

#include <memory>
#include <string_view>

#include <ChatService/Interfaces/IRoomMembersRepository.hpp>

class SQLiteRoomMembersRepository : public IRoomMembersRepository
{
public:
    explicit SQLiteRoomMembersRepository(std::shared_ptr<SQLite::Database> db)
        : db_(std::move(db))
    {
        db_->exec(CREATE_TABLE_COMMAND.data());
        db_->exec(CREATE_INDEX_USER_COMMAND.data());
    }

    void add(const protocol::RoomId roomId,
             const protocol::UserId userId,
             const protocol::rooms::MemberRole role,
             std::time_t joinedAt) override
    {
        SQLite::Statement insert(*db_, INSERT_MEMBER_COMMAND.data());
        insert.bind(1, static_cast<std::int64_t>(roomId));
        insert.bind(2, static_cast<std::int64_t>(userId));
        insert.bind(3, memberRoleToString(role));
        insert.bind(4, static_cast<std::int64_t>(joinedAt));
        insert.bind(5, 0); // lastReadMessageId стартует с 0
        insert.exec();
    }

    void remove(const protocol::RoomId roomId, const protocol::UserId userId) override
    {
        SQLite::Statement stmt(*db_, DELETE_MEMBER_COMMAND.data());
        stmt.bind(1, static_cast<std::int64_t>(roomId));
        stmt.bind(2, static_cast<std::int64_t>(userId));
        stmt.exec();
    }

    std::vector<RoomMember> membersOf(const protocol::RoomId roomId) const override
    {
        SQLite::Statement query(*db_, SELECT_MEMBERS_OF_ROOM.data());
        query.bind(1, static_cast<std::int64_t>(roomId));

        std::vector<RoomMember> result;
        while (query.executeStep())
            result.push_back(rowToMember(query));
        return result;
    }

    std::uint32_t countMembers(const protocol::RoomId roomId) const override
    {
        SQLite::Statement query(*db_, COUNT_MEMBERS_COMMAND.data());
        query.bind(1, static_cast<std::int64_t>(roomId));
        if (query.executeStep())
            return static_cast<std::uint32_t>(query.getColumn(0).getInt64());
        return 0;
    }

    std::vector<protocol::RoomId> roomIdsOfUser(const protocol::UserId userId) const override
    {
        SQLite::Statement query(*db_, SELECT_ROOMS_OF_USER.data());
        query.bind(1, static_cast<std::int64_t>(userId));

        std::vector<protocol::RoomId> result;
        while (query.executeStep())
            result.push_back(static_cast<protocol::RoomId>(query.getColumn(0).getInt64()));
        return result;
    }

    bool isMember(const protocol::RoomId roomId, protocol::UserId userId) const override
    {
        // Лёгкий запрос — только проверка существования, без полной выборки полей
        SQLite::Statement query(*db_, EXISTS_MEMBER_COMMAND.data());
        query.bind(1, static_cast<std::int64_t>(roomId));
        query.bind(2, static_cast<std::int64_t>(userId));
        return query.executeStep(); // true если строка нашлась
    }

    std::optional<RoomMember> get(const protocol::RoomId roomId, const protocol::UserId userId) const override
    {
        SQLite::Statement query(*db_, SELECT_MEMBER_COMMAND.data());
        query.bind(1, static_cast<std::int64_t>(roomId));
        query.bind(2, static_cast<std::int64_t>(userId));
        if (query.executeStep())
            return rowToMember(query);
        return std::nullopt;
    }

    void updateRole(const protocol::RoomId roomId,
                    const protocol::UserId userId,
                    const protocol::rooms::MemberRole role) override
    {
        SQLite::Statement stmt(*db_, UPDATE_ROLE_COMMAND.data());
        stmt.bind(1, memberRoleToString(role));
        stmt.bind(2, static_cast<std::int64_t>(roomId));
        stmt.bind(3, static_cast<std::int64_t>(userId));
        stmt.exec();
    }

    void updateLastRead(const protocol::RoomId roomId,
                        const protocol::UserId userId,
                        const protocol::MessageId lastReadMessageId) override
    {
        // Важный момент: используем MAX, чтобы не сдвигать позицию назад.
        // Если клиент по ошибке прислал старый id — позиция не уменьшится.
        SQLite::Statement stmt(*db_, UPDATE_LAST_READ_COMMAND.data());
        stmt.bind(1, static_cast<std::int64_t>(lastReadMessageId));
        stmt.bind(2, static_cast<std::int64_t>(roomId));
        stmt.bind(3, static_cast<std::int64_t>(userId));
        stmt.exec();
    }

private:
    static RoomMember rowToMember(SQLite::Statement &query)
    {
        RoomMember m;
        m.roomId = static_cast<protocol::RoomId>(query.getColumn(0).getInt64());
        m.userId = static_cast<protocol::UserId>(query.getColumn(1).getInt64());
        m.role = memberRoleFromString(query.getColumn(2).getString());
        m.joinedAt = static_cast<std::time_t>(query.getColumn(3).getInt64());
        m.lastReadMessageId = static_cast<protocol::MessageId>(query.getColumn(4).getInt64());
        return m;
    }

    static std::string memberRoleToString(protocol::rooms::MemberRole r)
    {
        return std::string(magic_enum::enum_name(r));
    }
    static protocol::rooms::MemberRole memberRoleFromString(const std::string &s)
    {
        auto casted = magic_enum::enum_cast<protocol::rooms::MemberRole>(s);
        return casted ? casted.value() : protocol::rooms::MemberRole::Member;
    }

    std::shared_ptr<SQLite::Database> db_;

    // ─── SQL ───────────────────────────────────────────────────────

    static constexpr std::string_view CREATE_TABLE_COMMAND =
        "CREATE TABLE IF NOT EXISTS roomMembers("
        "roomId             INTEGER NOT NULL,"
        "userId             INTEGER NOT NULL,"
        "role               TEXT    NOT NULL CHECK (role IN ('Owner','Admin','Member')),"
        "joinedAt           INTEGER NOT NULL,"
        "lastReadMessageId  INTEGER NOT NULL DEFAULT 0,"
        // Составной PK: пара (комната, юзер) уникальна — нельзя дважды добавить
        // одного юзера в одну комнату. Заодно даёт быстрый поиск по (roomId, userId).
        "PRIMARY KEY (roomId, userId),"
        "FOREIGN KEY (roomId) REFERENCES rooms(id) ON DELETE CASCADE,"
        "FOREIGN KEY (userId) REFERENCES users(id) ON DELETE CASCADE"
        ")";

    // PK уже покрывает запросы вида "WHERE roomId = ? AND userId = ?" и "WHERE roomId = ?".
    // Но для "какие комнаты у юзера X" нужен отдельный индекс с userId впереди.
    static constexpr std::string_view CREATE_INDEX_USER_COMMAND =
        "CREATE INDEX IF NOT EXISTS idx_room_members_user ON roomMembers(userId)";

    static constexpr std::string_view INSERT_MEMBER_COMMAND =
        "INSERT INTO roomMembers (roomId, userId, role, joinedAt, lastReadMessageId) "
        "VALUES (?, ?, ?, ?, ?)";

    static constexpr std::string_view DELETE_MEMBER_COMMAND =
        "DELETE FROM roomMembers WHERE roomId = ? AND userId = ?";

    static constexpr std::string_view SELECT_MEMBERS_OF_ROOM =
        "SELECT roomId, userId, role, joinedAt, lastReadMessageId "
        "FROM roomMembers WHERE roomId = ?";

    static constexpr std::string_view COUNT_MEMBERS_COMMAND =
        "SELECT COUNT(*) FROM roomMembers WHERE roomId = ?";

    // Использует индекс idx_room_members_user (userId впереди).
    static constexpr std::string_view SELECT_ROOMS_OF_USER =
        "SELECT roomId FROM roomMembers WHERE userId = ?";

    // Самый лёгкий способ проверить членство — SELECT 1 LIMIT 1.
    // Не тащим из строки никаких полей, просто проверяем "есть ли".
    static constexpr std::string_view EXISTS_MEMBER_COMMAND =
        "SELECT 1 FROM roomMembers WHERE roomId = ? AND userId = ? LIMIT 1";

    static constexpr std::string_view SELECT_MEMBER_COMMAND =
        "SELECT roomId, userId, role, joinedAt, lastReadMessageId "
        "FROM roomMembers WHERE roomId = ? AND userId = ?";

    static constexpr std::string_view UPDATE_ROLE_COMMAND =
        "UPDATE roomMembers SET role = ? WHERE roomId = ? AND userId = ?";

    // MAX гарантирует "только вперёд": если присылают меньший id —
    // позиция не откатывается.
    static constexpr std::string_view UPDATE_LAST_READ_COMMAND =
        "UPDATE roomMembers "
        "SET lastReadMessageId = MAX(lastReadMessageId, ?) "
        "WHERE roomId = ? AND userId = ?";
};
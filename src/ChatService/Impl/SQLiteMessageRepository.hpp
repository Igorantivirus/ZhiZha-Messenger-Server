#pragma once

#include "ChatService/Types/Message.hpp"
#include <ctime>
#include <memory>
#include <string_view>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Protocol/Types.hpp>

#include <ChatService/Interfaces/IMessageRepository.hpp>

class SQLiteMessageRepository : public IMessageRepository
{
public:
    explicit SQLiteMessageRepository(std::shared_ptr<SQLite::Database> db)
        : db_(std::move(db))
    {
        db_->exec(CREATE_TABLE_COMMAND.data());
        db_->exec(CREATE_INDEX_COMMAND.data());
    }

    protocol::MessageId create(const protocol::RoomId roomId,
                               const protocol::UserId fromUserId,
                               const std::string &text) override
    {
        // createdAt берём прямо здесь — это серверное время, не клиентское
        const std::int64_t createdAt = static_cast<std::int64_t>(std::time(nullptr));

        SQLite::Statement insert(*db_, INSERT_MESSAGE_COMMAND.data());
        insert.bind(1, static_cast<std::int64_t>(roomId));
        insert.bind(2, static_cast<std::int64_t>(fromUserId));
        insert.bind(3, text);
        insert.bind(4, createdAt);
        insert.exec();
        return static_cast<protocol::MessageId>(db_->getLastInsertRowid());
    }

    std::optional<Message> findById(const protocol::MessageId id) const override
    {
        SQLite::Statement query(*db_, SELECT_BY_ID.data());
        query.bind(1, static_cast<std::int64_t>(id));
        if (query.executeStep())
            return rowToMessage(query);
        return std::nullopt;
    }

    std::vector<Message> findAfter(const protocol::RoomId roomId,
                                   const protocol::MessageId afterId,
                                   const unsigned limit) const override
    {
        SQLite::Statement query(*db_, SELECT_AFTER.data());
        query.bind(1, static_cast<std::int64_t>(roomId));
        query.bind(2, static_cast<std::int64_t>(afterId));
        query.bind(3, static_cast<std::int64_t>(limit));

        std::vector<Message> result;
        result.reserve(limit);
        while (query.executeStep())
            result.push_back(rowToMessage(query));
        return result;
    }

    std::vector<Message> findBefore(const protocol::RoomId roomId,
                                    const protocol::MessageId beforeId,
                                    const unsigned limit) const override
    {
        SQLite::Statement query(*db_, SELECT_BEFORE.data());
        query.bind(1, static_cast<std::int64_t>(roomId));
        query.bind(2, static_cast<std::int64_t>(beforeId));
        query.bind(3, static_cast<std::int64_t>(limit));

        std::vector<Message> result;
        result.reserve(limit);
        while (query.executeStep())
            result.push_back(rowToMessage(query));
        return result;
    }

    std::vector<Message> findLatest(const protocol::RoomId roomId, const unsigned limit) const override
    {
        SQLite::Statement query(*db_, SELECT_LATEST.data());
        query.bind(1, static_cast<std::int64_t>(roomId));
        query.bind(2, static_cast<std::int64_t>(limit));

        std::vector<Message> result;
        result.reserve(limit);
        while (query.executeStep())
            result.push_back(rowToMessage(query));
        return result;
    }

    std::optional<Message> findLastMessageInRoom(const protocol::RoomId roomId) const override
    {
        SQLite::Statement query(*db_, SELECT_LAST_MESSAGE_FROM_ROOM.data());
        query.bind(1, static_cast<std::int64_t>(roomId));

        return query.executeStep() ? std::optional<Message>(rowToMessage(query)) : std::nullopt;
    }

    void removeAllInRoom(const protocol::RoomId roomId) override
    {
        SQLite::Statement stmt(*db_, DELETE_ALL_IN_ROOM_COMMAND.data());
        stmt.bind(1, static_cast<std::int64_t>(roomId));
        stmt.exec();
    }

private:
    static Message rowToMessage(SQLite::Statement &query)
    {
        Message m;
        m.id = static_cast<protocol::MessageId>(query.getColumn(0).getInt64());
        m.roomId = static_cast<protocol::RoomId>(query.getColumn(1).getInt64());
        m.fromUserId = static_cast<protocol::UserId>(query.getColumn(2).getInt64());
        m.text = query.getColumn(3).getString();
        m.createdAt = static_cast<std::time_t>(query.getColumn(4).getInt64());
        return m;
    }

    std::shared_ptr<SQLite::Database> db_;

    // ─── SQL ───────────────────────────────────────────────────────

    static constexpr const std::string_view CREATE_TABLE_COMMAND =
        "CREATE TABLE IF NOT EXISTS messages("
        "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "roomId      INTEGER NOT NULL,"
        "fromUserId  INTEGER NOT NULL,"
        "text        TEXT    NOT NULL,"
        "createdAt   INTEGER NOT NULL,"
        "FOREIGN KEY (roomId)     REFERENCES rooms(id),"
        "FOREIGN KEY (fromUserId) REFERENCES users(id)"
        ")";

    // Главный индекс — под все запросы вида "сообщения комнаты X с id > Y / < Y".
    // (roomId, id) обслуживает обе SELECT_AFTER и SELECT_BEFORE.
    static constexpr const std::string_view CREATE_INDEX_COMMAND =
        "CREATE INDEX IF NOT EXISTS idx_messages_room ON messages(roomId, id)";

    static constexpr const std::string_view INSERT_MESSAGE_COMMAND =
        "INSERT INTO messages (roomId, fromUserId, text, createdAt) "
        "VALUES (?, ?, ?, ?)";

    static constexpr const std::string_view SELECT_BY_ID =
        "SELECT id, roomId, fromUserId, text, createdAt "
        "FROM messages WHERE id = ?";

    // Хронологический порядок: ASC — старые сначала.
    // Клиент догружает и вставляет в конец списка.
    static constexpr const std::string_view SELECT_AFTER =
        "SELECT id, roomId, fromUserId, text, createdAt "
        "FROM messages "
        "WHERE roomId = ? AND id > ? "
        "ORDER BY id ASC "
        "LIMIT ?";

    // Скролл вверх: возвращаем от ближайшего к курсору вниз.
    // Клиент видит первый элемент списка как "следующее раньше известного" — удобно для UI.
    static constexpr const std::string_view SELECT_BEFORE =
        "SELECT id, roomId, fromUserId, text, createdAt "
        "FROM messages "
        "WHERE roomId = ? AND id < ? "
        "ORDER BY id DESC "
        "LIMIT ?";

    static constexpr const std::string_view DELETE_ALL_IN_ROOM_COMMAND =
        "DELETE FROM messages WHERE roomId = ?";

    // Последние limit сообщений комнаты по убыванию id.
    // Используется при первой загрузке: клиент не знает курсора.
    static constexpr const std::string_view SELECT_LATEST =
        "SELECT id, roomId, fromUserId, text, createdAt "
        "FROM messages "
        "WHERE roomId = ? "
        "ORDER BY id DESC "
        "LIMIT ?";

    static constexpr const std::string_view SELECT_LAST_MESSAGE_FROM_ROOM =
        "SELECT id, roomId, fromUserId, text, createdAt "
        "FROM messages "
        "WHERE roomId = ? "
        "ORDER BY id DESC "
        "LIMIT 1";
};
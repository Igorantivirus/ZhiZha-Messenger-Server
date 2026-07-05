#include <ChatService/Subservices/MessageRepository.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#include <Utils/Time.hpp>

namespace chat
{

namespace
{

// SQL.
// FK на rooms с ON DELETE CASCADE — удаление комнаты автоматически чистит
// её сообщения. Включение foreign_keys должно быть сделано на уровне
// общей БД (PRAGMA foreign_keys = ON при открытии).
constexpr std::string_view CREATE_TABLE =
    "CREATE TABLE IF NOT EXISTS messages ("
    "id         INTEGER PRIMARY KEY AUTOINCREMENT, "
    "roomId     INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE, "
    "fromUserId INTEGER NOT NULL, "
    "text       TEXT    NOT NULL, "
    "createdAt  INTEGER NOT NULL"
    ")";

constexpr std::string_view CREATE_INDEX_ROOM_ID =
    "CREATE INDEX IF NOT EXISTS idx_messages_room_id ON messages (roomId, id)";

constexpr std::string_view INSERT_MESSAGE =
    "INSERT INTO messages (roomId, fromUserId, text, createdAt) "
    "VALUES (?, ?, ?, ?)";

constexpr std::string_view SELECT_BY_ID =
    "SELECT id, roomId, fromUserId, text, createdAt FROM messages WHERE id = ?";

constexpr std::string_view DELETE_ALL_IN_ROOM =
    "DELETE FROM messages WHERE roomId = ?";

constexpr std::string_view SELECT_LAST_IN_ROOM =
    "SELECT id, roomId, fromUserId, text, createdAt "
    "FROM messages WHERE roomId = ? "
    "ORDER BY id DESC LIMIT 1";

constexpr std::string_view SELECT_AFTER =
    "SELECT id, roomId, fromUserId, text, createdAt "
    "FROM messages WHERE roomId = ? AND id > ? "
    "ORDER BY id ASC LIMIT ?";

constexpr std::string_view SELECT_BEFORE =
    "SELECT id, roomId, fromUserId, text, createdAt "
    "FROM messages WHERE roomId = ? AND id < ? "
    "ORDER BY id DESC LIMIT ?";

constexpr std::string_view SELECT_LATEST =
    "SELECT id, roomId, fromUserId, text, createdAt "
    "FROM messages WHERE roomId = ? "
    "ORDER BY id DESC LIMIT ?";

Message rowToMessage(SQLite::Statement &stmt)
{
    return Message{
        .id = static_cast<utils::MessageId>(stmt.getColumn(0).getInt64()),
        .roomId = static_cast<utils::RoomId>(stmt.getColumn(1).getInt64()),
        .fromUserId = static_cast<utils::UserId>(stmt.getColumn(2).getInt64()),
        .text = stmt.getColumn(3).getString(),
        .createdAt = static_cast<std::time_t>(stmt.getColumn(4).getInt64())};
}

std::vector<Message> fetchPage(SQLite::Database &db, std::string_view sql,
                               const utils::RoomId roomId, const std::int64_t cursor,
                               const unsigned limit)
{
    SQLite::Statement stmt(db, std::string(sql));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, cursor);
    stmt.bind(3, static_cast<std::int64_t>(limit));

    std::vector<Message> result;
    result.reserve(limit);
    while (stmt.executeStep())
        result.push_back(rowToMessage(stmt));
    return result;
}

} // namespace

MessageRepository::MessageRepository(SQLite::Database &db)
    : db_(db)
{
    db_.exec(std::string(CREATE_TABLE));
    db_.exec(std::string(CREATE_INDEX_ROOM_ID));
}

Message MessageRepository::create(const utils::RoomId roomId,
                                  const utils::UserId fromUserId,
                                  const std::string &text)
{
    const std::time_t now = utils::getCurrentTime();

    SQLite::Statement stmt(db_, std::string(INSERT_MESSAGE));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(fromUserId));
    stmt.bind(3, text);
    stmt.bind(4, static_cast<std::int64_t>(now));
    stmt.exec();

    return Message{
        .id = static_cast<utils::MessageId>(db_.getLastInsertRowid()),
        .roomId = roomId,
        .fromUserId = fromUserId,
        .text = text,
        .createdAt = now};
}

std::optional<Message> MessageRepository::findById(const utils::MessageId id) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_ID));
    stmt.bind(1, static_cast<std::int64_t>(id));
    if (!stmt.executeStep())
        return std::nullopt;
    return rowToMessage(stmt);
}

void MessageRepository::removeAllInRoom(const utils::RoomId roomId)
{
    SQLite::Statement stmt(db_, std::string(DELETE_ALL_IN_ROOM));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.exec();
}

std::optional<Message> MessageRepository::findLastMessageInRoom(const utils::RoomId roomId) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_LAST_IN_ROOM));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    if (!stmt.executeStep())
        return std::nullopt;
    return rowToMessage(stmt);
}

std::vector<Message> MessageRepository::findAfter(const utils::RoomId roomId,
                                                  const unsigned limit,
                                                  const utils::MessageId afterId) const
{
    return fetchPage(db_, SELECT_AFTER, roomId, static_cast<std::int64_t>(afterId), limit);
}

std::vector<Message> MessageRepository::findBefore(const utils::RoomId roomId,
                                                   const unsigned limit,
                                                   const utils::MessageId beforeId) const
{
    return fetchPage(db_, SELECT_BEFORE, roomId, static_cast<std::int64_t>(beforeId), limit);
}

std::vector<Message> MessageRepository::findLatest(const utils::RoomId roomId, const unsigned limit) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_LATEST));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(limit));

    std::vector<Message> result;
    result.reserve(limit);
    while (stmt.executeStep())
        result.push_back(rowToMessage(stmt));
    return result;
}

} // namespace chat

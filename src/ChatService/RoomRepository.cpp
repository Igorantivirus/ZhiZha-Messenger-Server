#include <ChatService/Subservices/RoomRepository.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include <Utils/Time.hpp>

namespace chat
{

namespace
{

// SQL.
constexpr std::string_view CREATE_TABLE =
    "CREATE TABLE IF NOT EXISTS rooms ("
    "id          INTEGER PRIMARY KEY AUTOINCREMENT, "
    "name        TEXT    NOT NULL, "
    "kind        TEXT    NOT NULL, "
    "joinPolicy  TEXT    NOT NULL, "
    "writePolicy TEXT    NOT NULL, "
    "createdAt   INTEGER NOT NULL"
    ")";

constexpr std::string_view INSERT_ROOM =
    "INSERT INTO rooms (name, kind, joinPolicy, writePolicy, createdAt) "
    "VALUES (?, ?, ?, ?, ?)";

constexpr std::string_view SELECT_BY_ID =
    "SELECT id, name, kind, joinPolicy, writePolicy, createdAt "
    "FROM rooms WHERE id = ?";

constexpr std::string_view DELETE_BY_ID =
    "DELETE FROM rooms WHERE id = ?";

constexpr std::string_view UPDATE_ROOM =
    "UPDATE rooms SET name = ?, kind = ?, joinPolicy = ?, writePolicy = ? WHERE id = ?";

// Поиск из UI: показываем только публично доступные комнаты, чтобы не
// светить названия закрытых/инвайт-онли. Жёсткое значение 'Public' выбрано
// сознательно: магическая строка ОК, потому что мы здесь сами и сериализуем
// JoinPolicy именно через magic_enum (см. toStr/fromStr).
constexpr std::string_view SELECT_BY_QUERY =
    "SELECT id, name, kind, joinPolicy, writePolicy, createdAt "
    "FROM rooms WHERE name LIKE ? AND joinPolicy = 'Public' LIMIT ?";

constexpr std::string_view SELECT_ROOMS_BY_ID =
    "SELECT id, name, kind, joinPolicy, writePolicy, createdAt "
    "FROM rooms WHERE id IN ";

// Конвертация enum'ов: имя перечислителя в строку через magic_enum.
template <typename Enum>
std::string toStr(const Enum e)
{
    return std::string(magic_enum::enum_name(e));
}

template <typename Enum>
Enum fromStr(const std::string &s, const Enum fallback)
{
    auto v = magic_enum::enum_cast<Enum>(s);
    return v.value_or(fallback);
}

Room rowToRoom(SQLite::Statement &stmt)
{
    return Room{
        .id = static_cast<utils::RoomId>(stmt.getColumn(0).getInt64()),
        .name = stmt.getColumn(1).getString(),
        .info = utils::RoomInfo{
                                .kind = fromStr<utils::RoomKind>(stmt.getColumn(2).getString(), utils::RoomKind::Group),
                                .joinPolicy = fromStr<utils::JoinPolicy>(stmt.getColumn(3).getString(), utils::JoinPolicy::Closed),
                                .writePolicy = fromStr<utils::WritePolicy>(stmt.getColumn(4).getString(), utils::WritePolicy::Everyone)},
        .createdAt = static_cast<std::time_t>(stmt.getColumn(5).getInt64())
    };
}

} // namespace

RoomRepository::RoomRepository(SQLite::Database &db)
    : db_(db)
{
    db_.exec(std::string(CREATE_TABLE));
}

Room RoomRepository::create(const std::string &name, const utils::RoomInfo info)
{
    const std::time_t now = utils::getCurrentTime();

    SQLite::Statement stmt(db_, std::string(INSERT_ROOM));
    stmt.bind(1, name);
    stmt.bind(2, toStr(info.kind));
    stmt.bind(3, toStr(info.joinPolicy));
    stmt.bind(4, toStr(info.writePolicy));
    stmt.bind(5, static_cast<std::int64_t>(now));
    stmt.exec();

    return Room{
        .id = static_cast<utils::RoomId>(db_.getLastInsertRowid()),
        .name = name,
        .info = info,
        .createdAt = now};
}

std::optional<Room> RoomRepository::findById(utils::RoomId id) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_ID));
    stmt.bind(1, static_cast<std::int64_t>(id));
    if (!stmt.executeStep())
        return std::nullopt;
    return rowToRoom(stmt);
}

std::vector<Room> RoomRepository::findByIds(std::span<const utils::RoomId> ids) const
{
    if (ids.empty())
        return {};

    // Динамически собираем placeholders: "?,?,?,..."
    std::string placeholders;
    placeholders.reserve(ids.size() * 2);
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0)
            placeholders.push_back(',');
        placeholders.push_back('?');
    }

    const std::string sql = std::string(SELECT_ROOMS_BY_ID) + '(' + placeholders + ')';

    SQLite::Statement stmt(db_, sql);
    int idx = 1;
    for (const auto id : ids)
        stmt.bind(idx++, static_cast<std::int64_t>(id));

    std::vector<Room> result;
    result.reserve(ids.size());
    while (stmt.executeStep())
        result.push_back(rowToRoom(stmt));
    return result;
}

void RoomRepository::remove(const utils::RoomId id)
{
    SQLite::Statement stmt(db_, std::string(DELETE_BY_ID));
    stmt.bind(1, static_cast<std::int64_t>(id));
    stmt.exec();
}

void RoomRepository::updateRoom(const utils::RoomId id, const std::string &name, const utils::RoomInfo info)
{
    SQLite::Statement stmt(db_, std::string(UPDATE_ROOM));
    stmt.bind(1, name);
    stmt.bind(2, toStr(info.kind));
    stmt.bind(3, toStr(info.joinPolicy));
    stmt.bind(4, toStr(info.writePolicy));
    stmt.bind(5, static_cast<std::int64_t>(id));
    stmt.exec();
}

std::vector<Room> RoomRepository::findByQuery(const std::string &query, const unsigned limit) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_QUERY));
    stmt.bind(1, "%" + query + "%");
    stmt.bind(2, static_cast<std::int64_t>(limit));

    std::vector<Room> result;
    result.reserve(limit);
    while (stmt.executeStep())
        result.push_back(rowToRoom(stmt));
    return result;
}

} // namespace chat

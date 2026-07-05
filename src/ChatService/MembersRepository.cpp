#include <ChatService/Subservices/MembersRepository.hpp>

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
// PK (roomId, userId) — один и тот же юзер не может быть участником комнаты дважды.
// FK на rooms с CASCADE — удаление комнаты автоматически чистит её состав.
constexpr std::string_view CREATE_TABLE =
    "CREATE TABLE IF NOT EXISTS roomMembers ("
    "roomId            INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE, "
    "userId            INTEGER NOT NULL, "
    "role              TEXT    NOT NULL, "
    "lastReadMessageId INTEGER NOT NULL DEFAULT 0, "
    "joinedAt          INTEGER NOT NULL, "
    "PRIMARY KEY (roomId, userId)"
    ")";

constexpr std::string_view CREATE_INDEX_USER =
    "CREATE INDEX IF NOT EXISTS idx_roomMembers_user ON roomMembers (userId, roomId)";

constexpr std::string_view INSERT_MEMBER =
    "INSERT INTO roomMembers (roomId, userId, role, lastReadMessageId, joinedAt) "
    "VALUES (?, ?, ?, 0, ?)";

constexpr std::string_view SELECT_BY_IDS =
    "SELECT roomId, userId, role, lastReadMessageId, joinedAt "
    "FROM roomMembers WHERE roomId = ? AND userId = ?";

constexpr std::string_view DELETE_BY_IDS =
    "DELETE FROM roomMembers WHERE roomId = ? AND userId = ?";

constexpr std::string_view DELETE_ALL_IN_ROOM =
    "DELETE FROM roomMembers WHERE roomId = ?";

constexpr std::string_view UPDATE_ROLE =
    "UPDATE roomMembers SET role = ? WHERE roomId = ? AND userId = ?";

constexpr std::string_view UPDATE_LAST_READ =
    "UPDATE roomMembers SET lastReadMessageId = ? WHERE roomId = ? AND userId = ?";

constexpr std::string_view SELECT_ROOM_IDS_FOR_USER_AFTER =
    "SELECT roomId FROM roomMembers "
    "WHERE userId = ? AND roomId > ? "
    "ORDER BY roomId ASC LIMIT ?";

constexpr std::string_view SELECT_MEMBERS_AFTER =
    "SELECT roomId, userId, role, lastReadMessageId, joinedAt "
    "FROM roomMembers WHERE roomId = ? AND userId > ? "
    "ORDER BY userId ASC LIMIT ?";

constexpr std::string_view SELECT_MEMBER_IDS_IN_ROOM =
    "SELECT userId FROM roomMembers WHERE roomId = ?";

constexpr std::string_view SELECT_ROOM_IDS_OF_USER =
    "SELECT roomId FROM roomMembers WHERE userId = ?";

constexpr std::string_view COUNT_MEMBERS_IN_ROOM =
    "SELECT COUNT(*) FROM roomMembers WHERE roomId = ?";

constexpr std::string_view EXISTS_MEMBER =
    "SELECT 1 FROM roomMembers WHERE roomId = ? AND userId = ? LIMIT 1";

std::string roleToString(const utils::MemberRole r)
{
    return std::string(magic_enum::enum_name(r));
}
utils::MemberRole roleFromString(const std::string &s)
{
    auto v = magic_enum::enum_cast<utils::MemberRole>(s);
    return v.value_or(utils::MemberRole::Member);
}

Member rowToMember(SQLite::Statement &stmt)
{
    return Member{
        .roomId = static_cast<utils::RoomId>(stmt.getColumn(0).getInt64()),
        .userId = static_cast<utils::UserId>(stmt.getColumn(1).getInt64()),
        .role = roleFromString(stmt.getColumn(2).getString()),
        .lastReadMessageId = static_cast<utils::MessageId>(stmt.getColumn(3).getInt64()),
        .joinedAt = static_cast<std::time_t>(stmt.getColumn(4).getInt64())};
}

} // namespace

MembersRepository::MembersRepository(SQLite::Database &db)
    : db_(db)
{
    db_.exec(std::string(CREATE_TABLE));
    db_.exec(std::string(CREATE_INDEX_USER));
}

Member MembersRepository::create(const utils::RoomId roomId,
                                 const utils::UserId userId,
                                 const utils::MemberRole role)
{
    const std::time_t now = utils::getCurrentTime();

    SQLite::Statement stmt(db_, std::string(INSERT_MEMBER));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(userId));
    stmt.bind(3, roleToString(role));
    stmt.bind(4, static_cast<std::int64_t>(now));
    stmt.exec();

    return Member{
        .roomId = roomId,
        .userId = userId,
        .role = role,
        .lastReadMessageId = 0,
        .joinedAt = now};
}

std::optional<Member> MembersRepository::findByIds(const utils::RoomId roomId, const utils::UserId userId) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_IDS));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(userId));
    if (!stmt.executeStep())
        return std::nullopt;
    return rowToMember(stmt);
}

void MembersRepository::removeById(const utils::RoomId roomId, const utils::UserId userId)
{
    SQLite::Statement stmt(db_, std::string(DELETE_BY_IDS));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(userId));
    stmt.exec();
}

void MembersRepository::removeAllInRoom(const utils::RoomId roomId)
{
    SQLite::Statement stmt(db_, std::string(DELETE_ALL_IN_ROOM));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.exec();
}

void MembersRepository::updateRole(const utils::RoomId roomId,
                                   const utils::UserId userId,
                                   const utils::MemberRole role)
{
    SQLite::Statement stmt(db_, std::string(UPDATE_ROLE));
    stmt.bind(1, roleToString(role));
    stmt.bind(2, static_cast<std::int64_t>(roomId));
    stmt.bind(3, static_cast<std::int64_t>(userId));
    stmt.exec();
}

void MembersRepository::updateLastMessageId(const utils::RoomId roomId,
                                            const utils::UserId userId,
                                            const utils::MessageId msgId)
{
    SQLite::Statement stmt(db_, std::string(UPDATE_LAST_READ));
    stmt.bind(1, static_cast<std::int64_t>(msgId));
    stmt.bind(2, static_cast<std::int64_t>(roomId));
    stmt.bind(3, static_cast<std::int64_t>(userId));
    stmt.exec();
}

std::vector<utils::RoomId> MembersRepository::findRoomIdsForUserAfter(const utils::UserId userId,
                                                                     const unsigned limit,
                                                                     const utils::RoomId afterId) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_ROOM_IDS_FOR_USER_AFTER));
    stmt.bind(1, static_cast<std::int64_t>(userId));
    stmt.bind(2, static_cast<std::int64_t>(afterId));
    stmt.bind(3, static_cast<std::int64_t>(limit));

    std::vector<utils::RoomId> result;
    result.reserve(limit);
    while (stmt.executeStep())
        result.push_back(static_cast<utils::RoomId>(stmt.getColumn(0).getInt64()));
    return result;
}

std::vector<Member> MembersRepository::findMembersAfter(const utils::RoomId roomId,
                                                       const unsigned limit,
                                                       const utils::UserId afterId) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_MEMBERS_AFTER));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(afterId));
    stmt.bind(3, static_cast<std::int64_t>(limit));

    std::vector<Member> result;
    result.reserve(limit);
    while (stmt.executeStep())
        result.push_back(rowToMember(stmt));
    return result;
}

std::vector<utils::UserId> MembersRepository::findMemberIdsInRoom(const utils::RoomId roomId) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_MEMBER_IDS_IN_ROOM));
    stmt.bind(1, static_cast<std::int64_t>(roomId));

    std::vector<utils::UserId> result;
    while (stmt.executeStep())
        result.push_back(static_cast<utils::UserId>(stmt.getColumn(0).getInt64()));
    return result;
}

std::vector<utils::RoomId> MembersRepository::roomIdsOfUser(const utils::UserId userId) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_ROOM_IDS_OF_USER));
    stmt.bind(1, static_cast<std::int64_t>(userId));

    std::vector<utils::RoomId> result;
    while (stmt.executeStep())
        result.push_back(static_cast<utils::RoomId>(stmt.getColumn(0).getInt64()));
    return result;
}

unsigned MembersRepository::findCountMembersById(const utils::RoomId roomId) const
{
    SQLite::Statement stmt(db_, std::string(COUNT_MEMBERS_IN_ROOM));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    if (!stmt.executeStep())
        return 0;
    return static_cast<unsigned>(stmt.getColumn(0).getInt64());
}

bool MembersRepository::isMember(const utils::RoomId roomId, const utils::UserId userId) const
{
    SQLite::Statement stmt(db_, std::string(EXISTS_MEMBER));
    stmt.bind(1, static_cast<std::int64_t>(roomId));
    stmt.bind(2, static_cast<std::int64_t>(userId));
    return stmt.executeStep();
}

} // namespace chat

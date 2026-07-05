#include <AuthService/Subservices/RefreshTokenStore.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace auth
{

namespace
{

// SQL.
constexpr std::string_view CREATE_TABLE =
    "CREATE TABLE IF NOT EXISTS refreshTokens ("
    "tokenHash TEXT PRIMARY KEY, "
    "userId    INTEGER NOT NULL, "
    "issuedAt  INTEGER NOT NULL, "
    "expiresAt INTEGER NOT NULL"
    ")";

constexpr std::string_view INSERT_OR_REPLACE =
    "INSERT OR REPLACE INTO refreshTokens (tokenHash, userId, issuedAt, expiresAt) "
    "VALUES (?, ?, ?, ?)";

constexpr std::string_view SELECT_BY_HASH =
    "SELECT userId, issuedAt, expiresAt FROM refreshTokens WHERE tokenHash = ?";

constexpr std::string_view DELETE_BY_HASH =
    "DELETE FROM refreshTokens WHERE tokenHash = ?";

constexpr std::string_view DELETE_BY_USER =
    "DELETE FROM refreshTokens WHERE userId = ?";

constexpr std::string_view DELETE_EXPIRED =
    "DELETE FROM refreshTokens WHERE expiresAt < ?";

// Деталь хранения. Прототип — std::hash. В проде — sha256.
// Снаружи никто про это не знает.
std::string hashToken(const std::string &token)
{
    static const std::hash<std::string> h;
    return std::to_string(h(token));
}

} // namespace

RefreshTokenStore::RefreshTokenStore(SQLite::Database &db)
    : db_(db)
{
    db_.exec(std::string(CREATE_TABLE));
}

void RefreshTokenStore::save(const std::string &refreshToken, RefreshRecord record)
{
    SQLite::Statement stmt(db_, std::string(INSERT_OR_REPLACE));
    stmt.bind(1, hashToken(refreshToken));
    stmt.bind(2, static_cast<std::int64_t>(record.userId));
    stmt.bind(3, static_cast<std::int64_t>(record.issuedAt));
    stmt.bind(4, static_cast<std::int64_t>(record.expiresAt));
    stmt.exec();
}

std::optional<RefreshRecord> RefreshTokenStore::find(const std::string &refreshToken) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_HASH));
    stmt.bind(1, hashToken(refreshToken));

    if (!stmt.executeStep())
        return std::nullopt;

    return RefreshRecord{
        .userId = static_cast<utils::UserId>(stmt.getColumn(0).getInt64()),
        .issuedAt = static_cast<std::time_t>(stmt.getColumn(1).getInt64()),
        .expiresAt = static_cast<std::time_t>(stmt.getColumn(2).getInt64())};
}

void RefreshTokenStore::remove(const std::string &refreshToken)
{
    SQLite::Statement stmt(db_, std::string(DELETE_BY_HASH));
    stmt.bind(1, hashToken(refreshToken));
    stmt.exec();
}

void RefreshTokenStore::removeAllForUser(utils::UserId userId)
{
    SQLite::Statement stmt(db_, std::string(DELETE_BY_USER));
    stmt.bind(1, static_cast<std::int64_t>(userId));
    stmt.exec();
}

void RefreshTokenStore::removeExpired(std::time_t now)
{
    SQLite::Statement stmt(db_, std::string(DELETE_EXPIRED));
    stmt.bind(1, static_cast<std::int64_t>(now));
    stmt.exec();
}

} // namespace auth

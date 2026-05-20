#pragma once

#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Auth/Interfaces/ITokenStore.hpp>

class SQLiteRefreshTokenStore : public ITokenStore
{
public:
    explicit SQLiteRefreshTokenStore(std::shared_ptr<SQLite::Database> db)
        : db_(std::move(db))
    {
        db_->exec(CREATE_TABLE_COMMAND.data());
    }

    void save(const std::string &refreshToken, RefreshRecord record) override
    {
        SQLite::Statement insert(*db_, INSERT_COMMAND.data());
        insert.bind(1, hashToken(refreshToken)); // в БД лежит хеш, не токен
        insert.bind(2, static_cast<int64_t>(record.userId));
        insert.bind(3, record.issuedAt);
        insert.bind(4, record.expiresAt);
        insert.exec();
    }

    std::optional<RefreshRecord> find(const std::string &refreshToken) const override
    {
        SQLite::Statement query(*db_, SELECT_BY_HASH_COMMAND.data());
        query.bind(1, hashToken(refreshToken));

        if (query.executeStep())
        {
            return RefreshRecord{
                .userId = static_cast<UserId>(query.getColumn(0).getInt64()),
                .issuedAt = query.getColumn(1).getInt64(),
                .expiresAt = query.getColumn(2).getInt64()};
        }
        return std::nullopt;
    }

    void remove(const std::string &refreshToken) override
    {
        SQLite::Statement stmt(*db_, DELETE_BY_HASH_COMMAND.data());
        stmt.bind(1, hashToken(refreshToken));
        stmt.exec();
    }

    void removeAllForUser(UserId userId) override
    {
        SQLite::Statement stmt(*db_, DELETE_BY_USER_COMMAND.data());
        stmt.bind(1, static_cast<int64_t>(userId));
        stmt.exec();
    }

    unsigned removeExpired(int64_t now) override
    {
        SQLite::Statement stmt(*db_, DELETE_EXPIRED_COMMAND.data());
        stmt.bind(1, now);
        return stmt.exec(); // exec() возвращает число изменённых строк
    }

private:
    // Деталь хранения. Прототип — std::hash. Прод — sha256.
    // Снаружи никто про это не знает.
    static std::string hashToken(const std::string &token)
    {
        static const std::hash<std::string> h;
        return std::to_string(h(token));
    }

    // Общая БД всего сервера — владение разделяется через shared_ptr.
    std::shared_ptr<SQLite::Database> db_;

    static constexpr std::string_view CREATE_TABLE_COMMAND =
        "CREATE TABLE IF NOT EXISTS refreshTokens("
        "tokenHash TEXT PRIMARY KEY," // ключ — хеш токена, не сам токен
        "userId    INTEGER NOT NULL,"
        "issuedAt  INTEGER NOT NULL,"
        "expiresAt INTEGER NOT NULL"
        ")";

    static constexpr std::string_view INSERT_COMMAND =
        "INSERT INTO refreshTokens (tokenHash, userId, issuedAt, expiresAt) "
        "VALUES (?, ?, ?, ?)";

    static constexpr std::string_view SELECT_BY_HASH_COMMAND =
        "SELECT userId, issuedAt, expiresAt FROM refreshTokens WHERE tokenHash = ?";

    static constexpr std::string_view DELETE_BY_HASH_COMMAND =
        "DELETE FROM refreshTokens WHERE tokenHash = ?";

    static constexpr std::string_view DELETE_BY_USER_COMMAND =
        "DELETE FROM refreshTokens WHERE userId = ?";

    static constexpr std::string_view DELETE_EXPIRED_COMMAND =
        "DELETE FROM refreshTokens WHERE expiresAt < ?";
};
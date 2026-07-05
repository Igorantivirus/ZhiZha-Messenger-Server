#pragma once

#include <ctime>
#include <optional>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Utils/Types.hpp>

#include <AuthService/Types/RefreshRecord.hpp>

namespace auth
{

// Refresh-токены лежат в БД — они должны переживать рестарт сервера.
// TTL знает TokenService — он подсчитывает expiresAt и кладёт сюда готовый
// RefreshRecord. Хранится хеш токена, не сам токен.
class RefreshTokenStore
{
public:
    explicit RefreshTokenStore(SQLite::Database &db);

    void save(const std::string &refreshToken, RefreshRecord record);
    std::optional<RefreshRecord> find(const std::string &refreshToken) const;

    void remove(const std::string &refreshToken);
    void removeAllForUser(utils::UserId userId);

    // Удаляет все записи с expiresAt < now. Для фонового чистильщика.
    void removeExpired(std::time_t now);

private:
    SQLite::Database &db_;
};

} // namespace auth

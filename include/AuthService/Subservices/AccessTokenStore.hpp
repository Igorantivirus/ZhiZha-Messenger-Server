#pragma once

#include <ctime>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <Utils/Types.hpp>

#include <AuthService/Types/AccessRecord.hpp>

namespace auth
{

// In-memory хранилище access-токенов: они короткоживущие и не должны
// переживать рестарт сервера. TTL знает TokenService — он подсчитывает
// expiresAt и кладёт сюда готовый AccessRecord.
class AccessTokenStore
{
public:
    AccessTokenStore() = default;

    void save(const std::string &accessToken, AccessRecord record);
    std::optional<AccessRecord> find(const std::string &accessToken) const;

    void remove(const std::string &accessToken);
    void removeAllForUser(utils::UserId userId);

    // Удаляет все записи с expiresAt < now. Для фонового чистильщика.
    void removeExpired(std::time_t now);

private:
    mutable std::shared_mutex mtx_;
    std::unordered_map<std::string, AccessRecord> records_;
};

} // namespace auth

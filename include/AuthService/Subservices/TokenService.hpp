#pragma once

#include <ctime>
#include <expected>
#include <string>

#include <Utils/Random.hpp>

#include <AuthService/Subservices/AccessTokenStore.hpp>
#include <AuthService/Subservices/RefreshTokenStore.hpp>
#include <AuthService/Types/AuthError.hpp>
#include <AuthService/Types/AuthSuccess.hpp>

namespace auth
{

// Сервис токенов: владеет ttl, генерирует пары access/refresh, кладёт их
// в свои stores. Stores ничего про ttl не знают — TokenService сам считает
// expiresAt = now + ttl и кладёт готовый record.
class TokenService
{
public:
    TokenService(const std::time_t accessTtl,
                 const std::time_t refreshTtl,
                 AccessTokenStore &accessStore,
                 RefreshTokenStore &refreshStore);

    AuthSuccess issuePair(const utils::UserId userId);

    // Ротация: refresh-токен одноразовый — старый удаляется, выдаётся новая пара.
    // Ошибки: InvalidToken (нет в БД), TokenExpired (истёк по expiresAt).
    std::expected<AuthSuccess, AuthError> refresh(const std::string &refreshToken);

    void revokeRefresh(const std::string &refreshToken);
    void revokeAllForUser(const utils::UserId userId);

    // Ошибки: InvalidToken (нет в store), TokenExpired (истёк).
    std::expected<utils::UserId, AuthError> validateAccess(const std::string &accessToken);

private:
    const std::time_t accessTtl_;
    const std::time_t refreshTtl_;

    AccessTokenStore &accessStore_;
    RefreshTokenStore &refreshStore_;

    utils::RandomString rand_;
};

} // namespace auth

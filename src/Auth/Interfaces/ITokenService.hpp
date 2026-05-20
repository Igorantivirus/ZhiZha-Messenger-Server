#pragma once

#include <optional>

#include <Auth/Types/TokenPair.hpp>
#include <Utils/Types.hpp>

class ITokenService
{
public:
    virtual ~ITokenService() = default;
    virtual TokenPair issuePair(UserId userId) = 0;                                   // создать новую сессию
    virtual std::optional<TokenPair> refresh(const std::string &refreshToken) = 0;    // обновить доступ
    virtual void revokeRefresh(const std::string &refreshToken) = 0;                  // закрыть сессию
    virtual void revokeAllForUser(UserId userId) = 0;                                 // отменить сессию для всех
    virtual std::optional<UserId> validateAccess(const std::string &accessToken) = 0; // проверить доступ
};
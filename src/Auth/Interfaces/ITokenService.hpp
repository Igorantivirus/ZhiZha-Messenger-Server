#pragma once

#include <optional>

#include <Auth/Types/AuthSuccess.hpp>
#include <Utils/Types.hpp>

class ITokenService
{
public:
    virtual ~ITokenService() = default;
    virtual AuthSuccess issuePair(UserId userId) = 0;                                  // создать новую сессию
    virtual std::optional<AuthSuccess> refresh(const std::string &refreshToken) = 0;   // обновить доступ
    virtual void revokeRefresh(const std::string &refreshToken) = 0;                   // закрыть сессию
    virtual void revokeAllForUser(UserId userId) = 0;                                  // отменить сессию для всех
    virtual std::optional<UserId> validateAccess(const std::string &accessToken) = 0;  // проверить доступ
};

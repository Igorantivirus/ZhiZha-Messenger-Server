#include <AuthService/Subservices/TokenService.hpp>

#include <Utils/Time.hpp>

namespace auth
{

namespace
{
constexpr std::size_t TOKEN_LENGTH = 32;
}

TokenService::TokenService(const std::time_t accessTtl,
                           const std::time_t refreshTtl,
                           AccessTokenStore &accessStore,
                           RefreshTokenStore &refreshStore)
    : accessTtl_(accessTtl),
      refreshTtl_(refreshTtl),
      accessStore_(accessStore),
      refreshStore_(refreshStore)
{
    rand_.setSettings(utils::StringSettings::allow());
    rand_.setDefaultLength(TOKEN_LENGTH);
}

AuthSuccess TokenService::issuePair(const utils::UserId userId)
{
    const std::time_t now = utils::getCurrentTime();

    std::string access = rand_.generate();
    accessStore_.save(access, AccessRecord{
                                  .userId = userId,
                                  .expiresAt = now + accessTtl_});

    std::string refresh = rand_.generate();
    refreshStore_.save(refresh, RefreshRecord{
                                    .userId = userId,
                                    .issuedAt = now,
                                    .expiresAt = now + refreshTtl_});

    return AuthSuccess{
        .userId = userId,
        .tokens = TokenPair{
            .access = std::move(access),
            .refresh = std::move(refresh)}};
}

std::expected<AuthSuccess, AuthError> TokenService::refresh(const std::string &refreshToken)
{
    auto record = refreshStore_.find(refreshToken);
    if (!record)
        return std::unexpected(AuthError::InvalidToken);

    const std::time_t now = utils::getCurrentTime();
    if (record->expiresAt < now)
    {
        refreshStore_.remove(refreshToken); // подчищаем протухший
        return std::unexpected(AuthError::TokenExpired);
    }

    // Ротация: старый refresh всегда инвалидируется. Утечёт — урон ограничен
    // одним использованием.
    refreshStore_.remove(refreshToken);

    return issuePair(record->userId);
}

void TokenService::revokeRefresh(const std::string &refreshToken)
{
    refreshStore_.remove(refreshToken);
}

void TokenService::revokeAllForUser(const utils::UserId userId)
{
    refreshStore_.removeAllForUser(userId);
    accessStore_.removeAllForUser(userId);
}

std::expected<utils::UserId, AuthError> TokenService::validateAccess(const std::string &accessToken)
{
    auto record = accessStore_.find(accessToken);
    if (!record)
        return std::unexpected(AuthError::InvalidToken);

    const std::time_t now = utils::getCurrentTime();
    if (record->expiresAt < now)
    {
        accessStore_.remove(accessToken); // подчищаем протухший
        return std::unexpected(AuthError::TokenExpired);
    }

    return record->userId;
}

} // namespace auth

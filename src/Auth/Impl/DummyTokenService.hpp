#pragma once

#include <Utils/Random.hpp>
#include <Utils/Time.hpp>

#include <Auth/Interfaces/IAccessTokenStore.hpp>
#include <Auth/Interfaces/ITokenService.hpp>
#include <Auth/Interfaces/ITokenStore.hpp>

class DummyTokenService : public ITokenService
{
public:
    DummyTokenService(ITokenStore &refreshStore,
                      IAccessTokenStore &accessStore,
                      int accessTtlSeconds,
                      int refreshTtlSeconds)
        : refreshStore_(refreshStore),
          accessStore_(accessStore),
          accessTtlSeconds_(accessTtlSeconds),
          refreshTtlSeconds_(refreshTtlSeconds)
    {
        rnd::StringSettings setts = rnd::StringSettings::allow();
        rand_.setSettings(setts);
        rand_.setDefaultLength(32);
    }

    AuthSuccess issuePair(UserId userId) override
    {
        std::time_t now = getCurrentTime();

        // Создаём access — кладём в свой in-memory store
        std::string access = rand_.generate();
        accessStore_.save(access, AccessRecord{
                                      .userId = userId,
                                      .expiresAt = now + accessTtlSeconds_});

        // Создаём refresh — кладём в SQLite store
        std::string refresh = rand_.generate();
        refreshStore_.save(refresh, RefreshRecord{
                                        .userId = userId,
                                        .issuedAt = now,
                                        .expiresAt = now + refreshTtlSeconds_});

        return AuthSuccess{
            .userId = userId,
            .tokens = TokenPair{
                .access = std::move(access),
                .refresh = std::move(refresh)},
            .accessTtl = accessTtlSeconds_,
            .refreshTtl = refreshTtlSeconds_};
    }

    std::optional<UserId> validateAccess(const std::string &accessToken) override
    {
        auto record = accessStore_.find(accessToken);
        if (!record)
            return std::nullopt; // токена нет

        std::time_t now = getCurrentTime();
        if (record->expiresAt < now)
        {                                     // истёк
            accessStore_.remove(accessToken); // заодно убираем мусор
            return std::nullopt;
        }

        return record->userId;
    }

    std::optional<AuthSuccess> refresh(const std::string &refreshToken) override
    {
        // 1. Ищем refresh в БД
        auto record = refreshStore_.find(refreshToken);
        if (!record)
            return std::nullopt;

        // 2. Проверяем срок
        std::time_t now = getCurrentTime();
        if (record->expiresAt < now)
        {
            refreshStore_.remove(refreshToken);
            return std::nullopt;
        }

        // 3. Старый refresh ОБЯЗАТЕЛЬНО инвалидируем.
        //    Каждый refresh — одноразовый. Утечёт — урон ограничен одним использованием.
        refreshStore_.remove(refreshToken);

        // 4. Выдаём новую пару тому же юзеру
        return issuePair(record->userId);
    }

    void revokeRefresh(const std::string &refreshToken) override
    {
        refreshStore_.remove(refreshToken);
    }

    void revokeAllForUser(UserId userId) override
    {
        refreshStore_.removeAllForUser(userId);
        accessStore_.removeAllForUser(userId);
    }

private:
    ITokenStore &refreshStore_;
    IAccessTokenStore &accessStore_;
    const int accessTtlSeconds_;
    const int refreshTtlSeconds_;

    rnd::RandomString rand_;
};
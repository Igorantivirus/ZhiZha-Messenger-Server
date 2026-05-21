#pragma once

#include <string>
#include <optional>

#include <Auth/Types/RefreshRecord.hpp>

class ITokenStore
{
public:
    virtual ~ITokenStore() = default;

    // Сохранить запись о выданном refresh
    virtual void save(const std::string &refreshToken, RefreshRecord record) = 0;

    // Найти запись по токену. nullopt если нет.
    virtual std::optional<RefreshRecord> find(const std::string &refreshToken) const = 0;

    // Удалить конкретный refresh.
    virtual void remove(const std::string &refreshToken) = 0;

    // Удалить все refresh указанного юзера (для logout-all).
    virtual void removeAllForUser(protocol::UserId userId) = 0;

    // Удалить всё, у чего expiresAt < now. Возвращает число удалённых.
    virtual unsigned removeExpired(int64_t now) = 0;
};
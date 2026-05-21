#pragma once

#include <ctime>
#include <optional>
#include <string>

#include <Protocol/Types.hpp>

#include <Auth/Types/AccessRecord.hpp>

class IAccessTokenStore
{
public:
    virtual ~IAccessTokenStore() = default;

    virtual void save(const std::string &accessToken, AccessRecord record) = 0;
    virtual std::optional<AccessRecord> find(const std::string &accessToken) const = 0;
    virtual void remove(const std::string &accessToken) = 0;
    virtual void removeAllForUser(protocol::UserId userId) = 0;
    virtual unsigned removeExpired(std::time_t now) = 0;
};
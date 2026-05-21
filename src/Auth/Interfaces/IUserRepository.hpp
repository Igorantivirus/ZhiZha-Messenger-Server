#pragma once

#include <optional>
#include <string>

#include <Auth/Types/User.hpp>
#include <Protocol/Types.hpp>

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<User> findUserByUsername(const std::string &username) const = 0;
    virtual std::optional<User> findUserById(const protocol::UserId id) const = 0;

    virtual protocol::UserId create(const std::string &username, const std::string& displayeName, const std::string &passwordHash) = 0;

    virtual bool updateUsername(const protocol::UserId id, const std::string newUsername) = 0;
    virtual bool updatePasswordHash(const protocol::UserId id, const std::string newPasswordHash) = 0;
};
#pragma once

#include <optional>
#include <string>

#include <Auth/Types/User.hpp>
#include <Utils/Types.hpp>

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<User> findUserByUsername(const std::string &username) const = 0;
    virtual std::optional<User> findUserById(const UserId id) const = 0;

    virtual UserId create(const std::string &username, const std::string& displayeName, const std::string &passwordHash) = 0;

    virtual bool updateUsername(const UserId id, const std::string newUsername) = 0;
    virtual bool updatePasswordHash(const UserId id, const std::string newPasswordHash) = 0;
};
#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include <Auth/Types/User.hpp>
#include <Protocol/Types.hpp>
#include <Protocol/Users.hpp>

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<User> findUserByUsername(const std::string &username) const = 0;
    virtual std::optional<User> findUserById(const protocol::UserId id) const = 0;
    virtual std::vector<User> findUsersByQuery(std::string query, unsigned limit) const = 0;

    virtual protocol::UserId create(const std::string &username, const std::string &displayeName, const std::string &passwordHash, const std::time_t birthDate, const protocol::users::Country country) = 0;

    virtual bool updateUsername(const protocol::UserId id, const std::string newUsername) = 0;
    virtual bool updatePasswordHash(const protocol::UserId id, const std::string newPasswordHash) = 0;
};
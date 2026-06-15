#pragma once

#include <ctime>
#include <string>

#include <Protocol/Types.hpp>
#include <Protocol/Users.hpp>

struct User
{
    protocol::UserId id;
    std::string username;
    std::string passwordHash;
    std::int64_t registerTime;
    std::string displayeName;
    std::time_t birthDate;
    protocol::users::Country country;
};
#pragma once

#include <ctime>
#include <string>

#include <Utils/Types.hpp>

namespace auth
{

struct User
{
    utils::UserId id;
    std::string passwordHash;

    std::string username;
    std::string displayName;
    utils::Country country;

    std::time_t registerTime;
    std::time_t birthDate;
};

} // namespace auth
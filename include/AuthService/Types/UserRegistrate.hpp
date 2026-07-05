#pragma once

#include <ctime>
#include <string>

#include <Utils/Types.hpp>

namespace auth
{

struct UserRegistrate
{
    std::string password;
    std::string username;
    std::string displayName;
    utils::Country country;
    std::time_t birthDate;
};

} // namespace auth
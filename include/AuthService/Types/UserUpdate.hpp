#pragma once

#include <ctime>
#include <string>

#include <Utils/Types.hpp>

namespace auth
{

struct UserUpdate
{
    std::string username;
    std::string displayName;
    utils::Country country;
    std::time_t birthDate;
};

} // namespace auth
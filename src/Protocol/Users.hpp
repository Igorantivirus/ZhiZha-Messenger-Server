#pragma once

#include <ctime>
#include <string>

#include "Types.hpp"

namespace protocol::users
{
struct MeResponse
{
    UserId userId;
    std::string username;
    std::string displayname;
    std::time_t registerTime;
};
struct UserResponse
{
    UserId userId;
    std::string username;
    std::string displayname;
};
} // namespace protocol::users
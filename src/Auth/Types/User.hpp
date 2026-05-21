#pragma once

#include <string>

#include <Protocol/Types.hpp>

struct User
{
    protocol::UserId id;
    std::string username;
    std::string passwordHash;
    std::int64_t registerTime;
    std::string displayeName;
};
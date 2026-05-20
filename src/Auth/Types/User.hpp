#pragma once

#include <string>

#include <Utils/Types.hpp>

struct User
{
    UserId id;
    std::string username;
    std::string passwordHash;
    std::int64_t registerTime;
    std::string displayeName;
};
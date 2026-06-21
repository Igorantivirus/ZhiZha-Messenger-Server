#pragma once

#include <ctime>
#include <string>

#include <ProtocolV1/Common/Types.hpp>

struct User
{
    protocol::UserId id;
    std::string username;
    std::string passwordHash;
    std::int64_t registerTime;
    std::string displayeName;
    std::time_t birthDate;
    protocol::Country country;
};
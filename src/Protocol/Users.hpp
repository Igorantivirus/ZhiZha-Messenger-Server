#pragma once

#include <ctime>
#include <string>
#include <unordered_map>

#include "Types.hpp"
#include "Types.hpp"

// вспомогательные структуры
namespace protocol::users
{
struct UserDisplayInfo
{
    std::string username;
    std::string displayname;
};
} // namespace protocol::users

// клиент -> сервер
namespace protocol::users
{
} // namespace protocol::users

// сервер -> клиент
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

struct UsersLoopByExampleResponse
{
    std::unordered_map<UserId, UserDisplayInfo> users;
};
} // namespace protocol::users
#pragma once

#include <ctime>
#include <string>

#include "Types.hpp"

// клиент -> сервер
namespace protocol::auth
{
struct RegisterRequest
{
    std::string username;
    std::string password;
    std::string displayName;
};

struct LoginRequest
{
    std::string username;
    std::string password;
};

struct RefreshRequest
{
    std::string refreshToken;
};

struct LogoutRequest
{
    std::string refreshToken;
};
} // namespace protocol::auth

// сервер -> клиент
namespace protocol::auth
{

// Единый успешный ответ register/login/refresh.
struct AuthSuccessResponse
{
    UserId userId;
    std::string accessToken;
    std::string refreshToken;
    std::time_t accessExpiresIn;  // секунд до истечения access
    std::time_t refreshExpiresIn; // секунд до истечения refresh
};

} // namespace protocol::auth

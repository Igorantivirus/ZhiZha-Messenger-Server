#pragma once

#include <ctime>
#include <string>

#include <nlohmann/json.hpp>

#include <Utils/Types.hpp>

namespace protocol::auth
{
// Все запросы ниже идут по гарантированно известным путям
// (/api/v1/auth/...), поэтому поле type им не нужно.

struct RegisterRequest
{
    std::string username;
    std::string password;
    std::string displayName;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RegisterRequest, username, password, displayName)

struct LoginRequest
{
    std::string username;
    std::string password;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginRequest, username, password)

struct RefreshRequest
{
    std::string refreshToken;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RefreshRequest, refreshToken)

struct LogoutRequest
{
    std::string refreshToken;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogoutRequest, refreshToken)

// Единый успешный ответ register/login/refresh.
struct AuthSuccessResponse
{
    UserId userId;
    std::string accessToken;
    std::string refreshToken;
    std::time_t accessExpiresIn;  // секунд до истечения access
    std::time_t refreshExpiresIn; // секунд до истечения refresh
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthSuccessResponse, userId, accessToken, refreshToken, accessExpiresIn, refreshExpiresIn)

} // namespace protocol::auth

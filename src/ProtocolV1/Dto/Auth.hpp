#pragma once

#include <ctime>
#include <string>

#include <ProtocolV1/Common/Types.hpp>
#include <ProtocolV1/Common/Parsing.hpp>
#include <ProtocolV1/Data/Users.hpp>

namespace protocol::dto
{

// Запрос на регистрацию
struct RegisterRequestDto
{
    std::string username;
    std::string password;
    std::string displayName;
    std::time_t birthDate;
    Country country;
};
PROTOCOL_JSON_SEREALIZE(RegisterRequestDto)

// Запрос на авторизацию
struct LoginRequestDto
{
    std::string username;
    std::string password;
};
PROTOCOL_JSON_SEREALIZE(LoginRequestDto)

// Запрос на обновление токенов
struct RefreshRequestDto
{
    std::string refreshToken;
};
PROTOCOL_JSON_SEREALIZE(RefreshRequestDto)

// Запрос на завершение сессии
struct LogoutRequestDto
{
    std::string refreshToken;
};
PROTOCOL_JSON_SEREALIZE(LogoutRequestDto)

// ООтвет на успех входа
struct AuthSuccessResponseDto
{
    UserId userId;
    std::string accessToken;
    std::string refreshToken;
};
PROTOCOL_JSON_SEREALIZE(AuthSuccessResponseDto)

} // namespace protocol::dto

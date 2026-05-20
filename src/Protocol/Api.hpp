#pragma once

#include <ctime>
#include <string>

#include <nlohmann/json.hpp>

namespace protocol::api
{

// Единый машинно-читаемый код ошибки. Может прийти как в ответе HTTP,
// так и в любой момент по WebSocket (внутри protocol::ws::ErrorMessage).
// Сериализуется в строку через NLOHMANN_JSON_SERIALIZE_ENUM ниже.
enum class ErrorCode
{
    unknown,            // нераспознанная ошибка (fallback при десериализации)
    internalError,      // что-то упало на сервере
    invalidFormat,      // тело запроса / сообщение не распарсилось
    unauthorized,       // нет/протух access-токен
    usernameTaken,      // ник занят
    invalidCredentials, // неверный логин или пароль
    weakPassword,       // пароль не прошёл валидацию
    usernameValidation, // ник не прошёл валидацию
    invalidToken,       // access-токен невалиден
    invalidRefreshToken,// refresh-токен невалиден или истёк
    unknownMessageType, // WS: неизвестный type
    notFound            // ресурс не найден
};

// camelCase-строки в JSON. unknown — первым, поэтому неизвестная строка
// при десериализации схлопывается именно в него.
NLOHMANN_JSON_SERIALIZE_ENUM(ErrorCode, {
    {ErrorCode::unknown, "unknown"},
    {ErrorCode::internalError, "internalError"},
    {ErrorCode::invalidFormat, "invalidFormat"},
    {ErrorCode::unauthorized, "unauthorized"},
    {ErrorCode::usernameTaken, "usernameTaken"},
    {ErrorCode::invalidCredentials, "invalidCredentials"},
    {ErrorCode::weakPassword, "weakPassword"},
    {ErrorCode::usernameValidation, "usernameValidation"},
    {ErrorCode::invalidToken, "invalidToken"},
    {ErrorCode::invalidRefreshToken, "invalidRefreshToken"},
    {ErrorCode::unknownMessageType, "unknownMessageType"},
    {ErrorCode::notFound, "notFound"},
})

// Тело ошибки для HTTP-ответов (на известном пути type не нужен).
struct ErrorResponse
{
    ErrorCode code;      // машинно-читаемый код
    std::string message; // человеческий текст для UI
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ErrorResponse, code, message)

// discovery-endpoint: клиент перед коннектом узнаёт, куда ходить.
struct InfoResponse
{
    std::string serverName;
    std::string version;
    std::string wsEndpoint;
    std::time_t accessTtl;  // секунды жизни access
    std::time_t refreshTtl; // секунды жизни refresh
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InfoResponse, serverName, version, wsEndpoint, accessTtl, refreshTtl)

} // namespace protocol::api

#pragma once

#include <cstdint>

namespace protocol
{

// Единый машинно-читаемый код ошибки. Может прийти как в ответе HTTP,
// так и в любой момент по WebSocket.
enum class ErrorCode : std::uint8_t
{
    unknown,             // нераспознанная ошибка (fallback при десериализации)
    internalError,       // что-то упало на сервере
    invalidFormat,       // тело запроса / сообщение не распарсилось
    unauthorized,        // нет/протух access-токен
    usernameTaken,       // ник занят
    invalidCredentials,  // неверный логин или пароль
    weakPassword,        // пароль не прошёл валидацию
    usernameValidation,  // ник не прошёл валидацию
    invalidToken,        // access-токен невалиден
    invalidRefreshToken, // refresh-токен невалиден или истёк
    unknownMessageType,  // WS: неизвестный type
    notFound             // ресурс не найден
};
} // namespace protocol
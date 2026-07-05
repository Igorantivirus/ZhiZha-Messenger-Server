#pragma once

#include <string>

#include <Protocol/Common/Parsing.hpp>

namespace protocol
{
enum class ErrorCode : std::uint8_t
{
    Unknown,            // нераспознанная ошибка (fallback при десериализации)
    InternalError,      // что-то упало на сервере
    InvalidFormat,      // тело запроса / сообщение не распарсилось
    NotFound,           // ресурс не найден
    Forbidden,          // действие запрещено политикой/правами
    UnknownMessageType, // WS: неизвестный type
    MissingParams,      // Пропущены обязательные параметры запроса

    // === Auth-specific ===
    InvalidRefreshToken, // refresh-токен невалиден или истёк
    InvalidToken,        // access-токен невалиден
    Unauthorized,        // нет/протух access-токен
    UsernameTaken,       // ник занят
    UsernameValidation,  // ник не прошёл валидацию
    InvalidCredentials,  // неверный логин или пароль
    WeakPassword,        // пароль не прошёл валидацию

    // === Chat-specific ===
    NotAMember,        // юзер не состоит в комнате
    WriteForbidden,    // запрещено писать (например, AdminsOnly + role=Member)
    EmptyMessage,      // пустой текст сообщения
    MessageTooLong,    // текст превышает лимит
    RoomNotFound,      // комната не существует
    InvalidDirectRoom, // некорректные параметры для Direct-комнаты
    EmptyRoomName,     // у обычной комнаты пустое имя
    MemberAlready      // пользователь уже состоит в комнате
};
PROTOCOL_JSON_SEREALIZE(ErrorCode)

struct ErrorResponse
{
    ErrorCode code;      // машинно-читаемый код
    std::string message; // человеческий текст для UI
};
PROTOCOL_JSON_SEREALIZE(ErrorResponse)

} // namespace protocol
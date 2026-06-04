#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "Rooms.hpp"

#include "ErrorCode.hpp"
#include "Types.hpp"

namespace protocol::ws
{

// ─────────────────────────────────────────────────────────────
// Лёгкий конверт: парсим только type, чтобы понять, что десериализовать дальше.
// ─────────────────────────────────────────────────────────────
enum class WsMessageType
{
    Unknown, // нераспознанный тип (fallback при десериализации)

    Ping,        // клиент -> сервер: проверка живости
    SendMessage, // клиент -> сервер: отправить сообщение в чат
    // Создание и покидание комнаты делаются через HTTP REST, не через WS.

    Pong,       // сервер -> клиент: ответ на ping
    Error,      // сервер -> клиент: ошибка (может прийти в любой момент)
    NewMessage, // сервер -> клиент: новое сообщение в чате
    UserLeft,   // сервер -> клиент: пользователь покинул комнату (событие)
    RoomCreated // сервер -> клиент: пользователя добавили в новую комнату (событие)
};
struct Envelope
{
    WsMessageType type = WsMessageType::Unknown;
};
} // namespace protocol::ws

// ─────────────────────────────────────────────────────────────
// Клиент → сервер
// ─────────────────────────────────────────────────────────────
namespace protocol::ws
{
struct Ping
{
    WsMessageType type = WsMessageType::Ping;
};

struct SendMessageRequest
{
    WsMessageType type = WsMessageType::SendMessage;
    RoomId roomId = 0;
    std::string text;
};
} // namespace protocol::ws

// ─────────────────────────────────────────────────────────────
// Сервер → клиент
// ─────────────────────────────────────────────────────────────
namespace protocol::ws
{

struct Pong
{
    WsMessageType type = WsMessageType::Pong;
};

struct ErrorMessage
{
    WsMessageType type = WsMessageType::Error;
    protocol::ErrorCode code = protocol::ErrorCode::Unknown;
    std::string message;
};

struct NewMessageEvent
{
    WsMessageType type = WsMessageType::NewMessage;
    MessageId messageId = 0;
    RoomId roomId = 0;
    UserId senderId = 0;
    std::string text;
    std::time_t createdAt = 0;
};

struct UserLeftEvent
{
    WsMessageType type = WsMessageType::UserLeft;
    RoomId roomId = 0;
    UserId userId = 0;
};

struct RoomCreatedEvent
{
    WsMessageType type = WsMessageType::RoomCreated;
    RoomId roomId = 0;
};

} // namespace protocol::ws

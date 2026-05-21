#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "RoomInfo.hpp"

#include "ErrorCode.hpp"
#include "Types.hpp"

namespace protocol::ws
{

// ─────────────────────────────────────────────────────────────
// Лёгкий конверт: парсим только type, чтобы понять, что десериализовать дальше.
// ─────────────────────────────────────────────────────────────
enum class WsMessageType
{
    unknown, // нераспознанный тип (fallback при десериализации)

    ping,        // клиент -> сервер: проверка живости
    sendMessage, // клиент -> сервер: отправить сообщение в чат
    createRoom,  // клиент -> сервер: создать комнату
    leaveRoom,   // клиент -> сервер: покинуть комнату

    pong,       // сервер -> клиент: ответ на ping
    error,      // сервер -> клиент: ошибка (может прийти в любой момент)
    newMessage, // сервер -> клиент: новое сообщение в чате
    userLeft,   // сервер -> клиент: пользователь покинул комнату
    roomCreated // сервер -> клиент: создана комната
};
struct Envelope
{
    WsMessageType type = WsMessageType::unknown;
};
} // namespace protocol::ws

// ─────────────────────────────────────────────────────────────
// Клиент → сервер
// ─────────────────────────────────────────────────────────────
namespace protocol::ws
{
struct Ping
{
    WsMessageType type = WsMessageType::ping;
};

struct SendMessageRequest
{
    WsMessageType type = WsMessageType::sendMessage;
    RoomId roomId = 0;
    std::string text;
};

struct CreateRoomRequest
{
    WsMessageType type = WsMessageType::createRoom;
    std::string roomName;
    RoomInfo roomInfo;
    std::vector<UserId> invitedUsers;
};

struct LeaveRoomRequest
{
    WsMessageType type = WsMessageType::leaveRoom;
    RoomId roomId = 0;
};
} // namespace protocol::ws

// ─────────────────────────────────────────────────────────────
// Сервер → клиент
// ─────────────────────────────────────────────────────────────
namespace protocol::ws
{

struct Pong
{
    WsMessageType type = WsMessageType::pong;
};

struct ErrorMessage
{
    WsMessageType type = WsMessageType::error;
    protocol::ErrorCode code = protocol::ErrorCode::unknown;
    std::string message;
};

struct NewMessageEvent
{
    WsMessageType type = WsMessageType::newMessage;
    MessageId messageId = 0;
    RoomId roomId = 0;
    UserId senderId = 0;
    std::string text;
    std::time_t createdAt = 0;
};

struct UserLeftEvent
{
    WsMessageType type = WsMessageType::userLeft;
    RoomId roomId = 0;
    UserId userId = 0;
};

struct RoomCreatedEvent
{
    WsMessageType type = WsMessageType::roomCreated;
    RoomId roomId = 0;
};

} // namespace protocol::ws
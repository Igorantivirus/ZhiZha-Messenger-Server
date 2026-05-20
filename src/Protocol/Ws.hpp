#pragma once

#include <ChatService/Types/RoomInfo.hpp>
#include <string>

#include <nlohmann/json.hpp>

#include <Protocol/Api.hpp>
#include <Utils/Types.hpp>

// По WebSocket заранее неизвестно, какая структура придёт следующей,
// поэтому каждое сообщение несёт дискриминатор type. Формат плоский:
//   { "type": "sendMessage", ...поля... }
// Диспетчеризация: прочитать "type" -> WsMessageType -> десериализовать
// в конкретную структуру.
namespace protocol::ws
{

enum class WsMessageType
{
    unknown,     // нераспознанный тип (fallback при десериализации)
    ping,        // клиент -> сервер: проверка живости
    pong,        // сервер -> клиент: ответ на ping
    error,       // сервер -> клиент: ошибка (может прийти в любой момент)
    sendMessage, // клиент -> сервер: отправить сообщение в чат (заглушка под ChatService)
    newMessage,  // сервер -> клиент: новое сообщение в чате (заглушка под ChatService)
    createRoom,  // клиент -> сервер: создать комнату
    leaveRoom,   // клиент -> сервер: покинуть комнату
    userLeft,    // сервер -> клиент: пользователь покинул комнату
    roomCreated  // сервер -> клиент: создана комната
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    WsMessageType,
    {
        {WsMessageType::unknown,     "unknown"    },
        {WsMessageType::ping,        "ping"       },
        {WsMessageType::pong,        "pong"       },
        {WsMessageType::error,       "error"      },

        {WsMessageType::sendMessage, "sendMessage"},
        {WsMessageType::createRoom,  "createRoom" },
        {WsMessageType::leaveRoom,   "leaveRoom"  },

        {WsMessageType::newMessage,  "newMessage" },
        {WsMessageType::userLeft,    "userLeft"   },
        {WsMessageType::roomCreated, "roomCreated"},
})

// Лёгкий конверт: парсим только type, чтобы понять, что десериализовать дальше.
struct Envelope
{
    WsMessageType type = WsMessageType::unknown;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Envelope, type)

struct Ping
{
    WsMessageType type = WsMessageType::ping;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Ping, type)

struct Pong
{
    WsMessageType type = WsMessageType::pong;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Pong, type)

// Ошибка, отправляемая по WS. Переиспользует общий protocol::api::ErrorCode.
struct ErrorMessage
{
    WsMessageType type = WsMessageType::error;
    protocol::api::ErrorCode code = protocol::api::ErrorCode::unknown;
    std::string message;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ErrorMessage, type, code, message)

// --- Заглушки под будущий ChatService ---

struct SendMessageRequest
{
    WsMessageType type = WsMessageType::sendMessage;
    RoomId roomId = 0;
    std::string text;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageRequest, type, roomId, text)

struct CreateRoomRequest
{
    WsMessageType type = WsMessageType::createRoom;
    std::string roomName;
    info::RoomInfo roomInfo;
    std::vector<UserId> invitedUsers;
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    info::RoomKind,
    {
        {info::RoomKind::Channel, "chanel"},
        {info::RoomKind::Direct,  "direct"},
        {info::RoomKind::Group,   "group" }
})
NLOHMANN_JSON_SERIALIZE_ENUM(
    info::JoinPolicy,
    {
        {info::JoinPolicy::ByAdmin,  "by-admin" },
        {info::JoinPolicy::ByMember, "by-member"},
        {info::JoinPolicy::Closed,   "closed"   },
        {info::JoinPolicy::Public,   "public"   }
})
NLOHMANN_JSON_SERIALIZE_ENUM(
    info::WritePolicy,
    {
        {info::WritePolicy::AdminsOnly, "admins-only"},
        {info::WritePolicy::Everyone,   "everyone"   }
})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(info::RoomInfo, kind, joinPolicy, writePolicy)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateRoomRequest, type, roomName, roomInfo)

struct LeaveRoomRequest
{
    WsMessageType type = WsMessageType::leaveRoom;
    RoomId roomId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LeaveRoomRequest, type, roomId)

struct NewMessageEvent
{
    WsMessageType type = WsMessageType::newMessage;
    MessageId messageId;
    RoomId roomId = 0;
    UserId senderId = 0;
    std::string text;
    std::time_t createdAt = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NewMessageEvent, type, messageId, roomId, senderId, text, createdAt)

struct UserLeftEvent
{
    WsMessageType type = WsMessageType::userLeft;
    RoomId roomId;
    UserId userId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserLeftEvent, type, roomId, userId)

struct RoomCreatedEvent
{
    WsMessageType type = WsMessageType::roomCreated;
    RoomId roomId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomCreatedEvent, type, roomId)

} // namespace protocol::ws

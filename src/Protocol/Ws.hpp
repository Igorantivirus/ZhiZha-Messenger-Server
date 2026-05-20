#pragma once

#include <ctime>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <ChatService/Types/RoomInfo.hpp>
#include <Protocol/Api.hpp>
#include <Utils/Types.hpp>

// По WebSocket заранее неизвестно, какая структура придёт следующей,
// поэтому каждое сообщение несёт дискриминатор type. Формат плоский:
//   { "type": "sendMessage", ...поля... }
// Диспетчеризация: прочитать "type" -> WsMessageType -> десериализовать
// в конкретную структуру.
//
// Порядок в файле: enum -> структуры (сначала клиент->сервер, затем
// сервер->клиент) -> все JSON-макросы единым блоком внизу.
namespace protocol::ws
{

enum class WsMessageType
{
    unknown,     // нераспознанный тип (fallback при десериализации)
    ping,        // клиент -> сервер: проверка живости
    pong,        // сервер -> клиент: ответ на ping
    error,       // сервер -> клиент: ошибка (может прийти в любой момент)
    sendMessage, // клиент -> сервер: отправить сообщение в чат
    createRoom,  // клиент -> сервер: создать комнату
    leaveRoom,   // клиент -> сервер: покинуть комнату
    newMessage,  // сервер -> клиент: новое сообщение в чате
    userLeft,    // сервер -> клиент: пользователь покинул комнату
    roomCreated  // сервер -> клиент: создана комната
};

// ─────────────────────────────────────────────────────────────
// Лёгкий конверт: парсим только type, чтобы понять, что десериализовать дальше.
// ─────────────────────────────────────────────────────────────

struct Envelope
{
    WsMessageType type = WsMessageType::unknown;
};

// ─────────────────────────────────────────────────────────────
// Клиент → сервер
// ─────────────────────────────────────────────────────────────

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
    info::RoomInfo roomInfo;
    std::vector<UserId> invitedUsers;
};

struct LeaveRoomRequest
{
    WsMessageType type = WsMessageType::leaveRoom;
    RoomId roomId = 0;
};

// ─────────────────────────────────────────────────────────────
// Сервер → клиент
// ─────────────────────────────────────────────────────────────

struct Pong
{
    WsMessageType type = WsMessageType::pong;
};

// Ошибка, отправляемая по WS. Переиспользует общий protocol::api::ErrorCode.
struct ErrorMessage
{
    WsMessageType type = WsMessageType::error;
    protocol::api::ErrorCode code = protocol::api::ErrorCode::unknown;
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

// ─────────────────────────────────────────────────────────────
// JSON-сериализация
// ─────────────────────────────────────────────────────────────

// Сериализация типов из namespace info обязана жить в этом же namespace,
// иначе ADL не найдёт to_json/from_json при сериализации RoomInfo.
namespace info
{
NLOHMANN_JSON_SERIALIZE_ENUM(RoomKind, {
    {RoomKind::Direct, "direct"},
    {RoomKind::Group, "group"},
    {RoomKind::Channel, "channel"},
})
NLOHMANN_JSON_SERIALIZE_ENUM(JoinPolicy, {
    {JoinPolicy::Closed, "closed"},
    {JoinPolicy::ByMember, "byMember"},
    {JoinPolicy::ByAdmin, "byAdmin"},
    {JoinPolicy::Public, "public"},
})
NLOHMANN_JSON_SERIALIZE_ENUM(WritePolicy, {
    {WritePolicy::Everyone, "everyone"},
    {WritePolicy::AdminsOnly, "adminsOnly"},
})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomInfo, kind, joinPolicy, writePolicy)
} // namespace info

namespace protocol::ws
{
NLOHMANN_JSON_SERIALIZE_ENUM(WsMessageType, {
    {WsMessageType::unknown, "unknown"},
    {WsMessageType::ping, "ping"},
    {WsMessageType::pong, "pong"},
    {WsMessageType::error, "error"},
    {WsMessageType::sendMessage, "sendMessage"},
    {WsMessageType::createRoom, "createRoom"},
    {WsMessageType::leaveRoom, "leaveRoom"},
    {WsMessageType::newMessage, "newMessage"},
    {WsMessageType::userLeft, "userLeft"},
    {WsMessageType::roomCreated, "roomCreated"},
})

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Envelope, type)

// клиент -> сервер
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Ping, type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageRequest, type, roomId, text)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateRoomRequest, type, roomName, roomInfo, invitedUsers)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LeaveRoomRequest, type, roomId)

// сервер -> клиент
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Pong, type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ErrorMessage, type, code, message)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NewMessageEvent, type, messageId, roomId, senderId, text, createdAt)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserLeftEvent, type, roomId, userId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomCreatedEvent, type, roomId)

} // namespace protocol::ws

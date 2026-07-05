#pragma once

#include <string>

#include <Protocol/Common/Error.hpp>
#include <Protocol/Common/Parsing.hpp>
#include <Protocol/Data/Rooms.hpp>
#include <Protocol/Data/Users.hpp>

namespace protocol::ws
{

constexpr const char wsEndpoint[] = "/ws";

enum WsEventType : std::uint8_t
{
    // Универсальные
    Unknown,
    Error,
    Ping,
    Pong,
    // Сервер -> Клиент
    SendingNewMessage,
    MessageAck,
    RoomCreated,
    RoomDeleted,
    RoomUpdated,
    UserJoin,
    UserLeft,
    UserUpdate,
    // Клиент -> Сервер
    DispatcinghNewMessage
};

/////////////////////////////
// Универсальные сообщения //
/////////////////////////////

struct ErrorEvent
{
    WsEventType type = WsEventType::Error;
    ErrorCode error;
    std::string msg;
};
PROTOCOL_JSON_SEREALIZE(ErrorEvent)

struct PingEvent
{
    WsEventType type = WsEventType::Ping;
};
PROTOCOL_JSON_SEREALIZE(PingEvent)

struct PongEvent
{
    WsEventType type = WsEventType::Pong;
};
PROTOCOL_JSON_SEREALIZE(PongEvent)

//////////////////////
// Сервер -> Клиент //
//////////////////////

struct SendingNewMessageEvent
{
    WsEventType type = WsEventType::SendingNewMessage;
    RoomId roomId;
    MessageId messageId;
    UserId senderId;
    std::string text;
    std::time_t createdAt;
};
PROTOCOL_JSON_SEREALIZE(SendingNewMessageEvent)

struct MessageAckEvent
{
    WsEventType type = WsEventType::MessageAck;
    MessageId localMessageId;
    MessageId globalMessageId;
    RoomId roomId;
    std::time_t createAt;
};
PROTOCOL_JSON_SEREALIZE(MessageAckEvent)

struct RoomCreatedEvent
{
    WsEventType type = WsEventType::RoomCreated;
    data::Room room;
};
PROTOCOL_JSON_SEREALIZE(RoomCreatedEvent)

struct RoomDeletedEvent
{
    WsEventType type = WsEventType::RoomDeleted;
    RoomId roomId;
};
PROTOCOL_JSON_SEREALIZE(RoomDeletedEvent)

struct RoomUpdatedEvent
{
    WsEventType type = WsEventType::RoomUpdated;
    data::Room room;
};
PROTOCOL_JSON_SEREALIZE(RoomUpdatedEvent)

struct UserJoinEvent
{
    WsEventType type = WsEventType::UserJoin;
    RoomId roomId = 0;
    UserId userId = 0;
};
PROTOCOL_JSON_SEREALIZE(UserJoinEvent)

struct UserLeftEvent
{
    WsEventType type = WsEventType::UserLeft;
    RoomId roomId = 0;
    UserId userId = 0;
};
PROTOCOL_JSON_SEREALIZE(UserLeftEvent)

struct UserUpdateEvent
{
    WsEventType type = WsEventType::UserUpdate;
    UserId userId;
    data::UserDisplayInfo info;
};
PROTOCOL_JSON_SEREALIZE(UserUpdateEvent)

//////////////////////
// Клиент -> Сервер //
//////////////////////

struct DispatcinghNewMessageEvent
{
    WsEventType type = WsEventType::DispatcinghNewMessage;
    RoomId roomId;
    MessageId localMessageId;
    std::string text;
};
PROTOCOL_JSON_SEREALIZE(DispatcinghNewMessageEvent)

} // namespace protocol::ws
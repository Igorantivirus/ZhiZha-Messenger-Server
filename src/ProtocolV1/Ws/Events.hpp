#pragma once

#include <string>

#include <ProtocolV1/Common/Error.hpp>
#include <ProtocolV1/Common/Parsing.hpp>
#include <ProtocolV1/Data/Rooms.hpp>
#include <ProtocolV1/Data/Users.hpp>

namespace protocol::ws
{

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

template <WsEventType value>
struct WsEvent
{
    WsEventType type = value;
};

/////////////////////////////
// Универсальные сообщения //
/////////////////////////////

struct ErrorEvent : public WsEvent<WsEventType::Error>
{
    ErrorCode error;
    std::string msg;
};
PROTOCOL_JSON_SEREALIZE(ErrorEvent)

struct PingEvent : public WsEvent<WsEventType::Ping>
{
};
PROTOCOL_JSON_SEREALIZE(PingEvent)

struct PongEvent : public WsEvent<WsEventType::Pong>
{
};
PROTOCOL_JSON_SEREALIZE(PongEvent)

//////////////////////
// Сервер -> Клиент //
//////////////////////

struct SendingNewMessageEvent : public WsEvent<WsEventType::SendingNewMessage>
{
    RoomId roomId;
    MessageId messageId;
    UserId senderId;
    std::string text;
    std::time_t createdAt;
};
PROTOCOL_JSON_SEREALIZE(SendingNewMessageEvent)

struct MessageAckEvent : public WsEvent<WsEventType::MessageAck>
{
    MessageId localMessageId;
    MessageId globalMessageId;
    RoomId roomId;
    std::time_t createAt;
};
PROTOCOL_JSON_SEREALIZE(MessageAckEvent)

struct RoomCreatedEvent : public WsEvent<WsEventType::RoomCreated>
{
    data::Room room;
};
PROTOCOL_JSON_SEREALIZE(RoomCreatedEvent)

struct RoomDeletedEvent : public WsEvent<WsEventType::RoomDeleted>
{
    RoomId roomId;
};
PROTOCOL_JSON_SEREALIZE(RoomDeletedEvent)

struct RoomUpdatedEvent : public WsEvent<WsEventType::RoomUpdated>
{
    data::Room room;
};
PROTOCOL_JSON_SEREALIZE(RoomUpdatedEvent)

struct UserJoinEvent : public WsEvent<WsEventType::UserJoin>
{
    RoomId roomId = 0;
    UserId userId = 0;
};
PROTOCOL_JSON_SEREALIZE(UserJoinEvent)

struct UserLeftEvent : public WsEvent<WsEventType::UserLeft>
{
    RoomId roomId = 0;
    UserId userId = 0;
};
PROTOCOL_JSON_SEREALIZE(UserLeftEvent)

struct UserUpdateEvent : public WsEvent<WsEventType::UserUpdate>
{
    UserId userId;
    data::UserDisplayInfo info;
};
PROTOCOL_JSON_SEREALIZE(UserUpdateEvent)

//////////////////////
// Клиент -> Сервер //
//////////////////////

struct DispatcinghNewMessageEvent : public WsEvent<WsEventType::DispatcinghNewMessage>
{
    RoomId roomId;
    MessageId localMessageId;
    std::string text;
};
PROTOCOL_JSON_SEREALIZE(DispatcinghNewMessageEvent)

} // namespace protocol::ws
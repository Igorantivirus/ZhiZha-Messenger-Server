#pragma once

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "Api.hpp"
#include "Auth.hpp"
#include "ErrorCode.hpp"
#include "Messages.hpp"
#include "Rooms.hpp"
#include "Users.hpp"
#include "Ws.hpp"

// Конвертация enum'ов в/из JSON через magic_enum: строковое представление
// совпадает с именем перечислителя (например WsMessageType::Ping <-> "Ping").
// Макрос объявляется в namespace перечислителя, чтобы его нашёл ADL.
#ifndef NLOHMANN_JSON_MAGIC_ENUM
#define NLOHMANN_JSON_MAGIC_ENUM(EnumType)                                             \
    inline void to_json(nlohmann::json &j, const EnumType &e)                          \
    {                                                                                  \
        auto name = magic_enum::enum_name(e);                                          \
        if (name.empty())                                                              \
        {                                                                              \
            throw nlohmann::json::other_error::create(                                 \
                501,                                                                   \
                "Cannot serialize " #EnumType ": value " +                             \
                    std::to_string(static_cast<std::underlying_type_t<EnumType>>(e)) + \
                    " is not a named enumerator",                                      \
                nullptr);                                                              \
        }                                                                              \
        j = std::string(name);                                                         \
    }                                                                                  \
                                                                                       \
    inline void from_json(const nlohmann::json &j, EnumType &e)                        \
    {                                                                                  \
        if (!j.is_string())                                                            \
        {                                                                              \
            throw nlohmann::json::type_error::create(                                  \
                302,                                                                   \
                "Cannot deserialize " #EnumType ": expected string, got " +            \
                    std::string(j.type_name()),                                        \
                &j);                                                                   \
        }                                                                              \
        const auto &s = j.get_ref<const std::string &>();                              \
        auto value = magic_enum::enum_cast<EnumType>(s);                               \
        if (!value.has_value())                                                        \
        {                                                                              \
            throw nlohmann::json::other_error::create(                                 \
                501,                                                                   \
                "Cannot deserialize " #EnumType ": '" + s +                            \
                    "' is not a valid enumerator",                                     \
                &j);                                                                   \
        }                                                                              \
        e = *value;                                                                    \
    }
#endif

namespace protocol
{

template <class DtoType>
bool parseJsonTo(const std::string &str, DtoType &dto)
{
    try
    {
        nlohmann::json::parse(str).get_to(dto);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Единый код ошибки — общий для HTTP и WS.
NLOHMANN_JSON_MAGIC_ENUM(ErrorCode)

} // namespace protocol

namespace protocol::api
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ErrorResponse, code, message)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InfoResponse, serverName, version, wsEndpoint, accessTtl, refreshTtl)
} // namespace protocol::api

namespace protocol::auth
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RegisterRequest, username, password, displayName)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginRequest, username, password)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RefreshRequest, refreshToken)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogoutRequest, refreshToken)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthSuccessResponse, userId, accessToken, refreshToken, accessExpiresIn, refreshExpiresIn)
} // namespace protocol::auth

namespace protocol::rooms
{
NLOHMANN_JSON_MAGIC_ENUM(RoomKind)
NLOHMANN_JSON_MAGIC_ENUM(JoinPolicy)
NLOHMANN_JSON_MAGIC_ENUM(WritePolicy)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomInfo, kind, joinPolicy, writePolicy)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Room, id, name, info)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Member, userId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateRoomRequest, roomName, roomInfo, invitedUsers)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InviteMemberRequest, invitedId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateRoomResponse, roomId)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GetRoomsResponse, rooms)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomInfoResponse, room)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomsMembersInfoResponse, members)
} // namespace protocol::rooms

namespace protocol::messages
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Message, id, fromUserId, text, createdAt)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MessagesResponse, roomId, messages, hasMore)
} // namespace protocol::messages

namespace protocol::users
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDisplayInfo, username, displayname)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeResponse, userId, username, displayname, registerTime)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserResponse, userId, username, displayname)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UsersLoopByExampleResponse, users)
} // namespace protocol::users

namespace protocol::ws
{
NLOHMANN_JSON_MAGIC_ENUM(WsMessageType)

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

#ifdef NLOHMANN_JSON_MAGIC_ENUM
#undef NLOHMANN_JSON_MAGIC_ENUM
#endif

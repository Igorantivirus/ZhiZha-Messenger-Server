#pragma once

#include <nlohmann/json.hpp>

#include "Api.hpp"
#include "Auth.hpp"
#include "Ws.hpp"

namespace protocol
{

template <class DtoType>
bool parseJsonTo(const std::string &str, DtoType &dto)
{
    try
    {
        nlohmann::json json = nlohmann::json::parse(str);
        json.get_to(dto);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace protocol

namespace protocol::api
{
// camelCase-строки в JSON. unknown — первым, поэтому неизвестная строка
// при десериализации схлопывается именно в него.
NLOHMANN_JSON_SERIALIZE_ENUM(ErrorCode,
                             {
                                 {ErrorCode::unknown,             "unknown"            },
                                 {ErrorCode::internalError,       "internalError"      },
                                 {ErrorCode::invalidFormat,       "invalidFormat"      },
                                 {ErrorCode::unauthorized,        "unauthorized"       },
                                 {ErrorCode::usernameTaken,       "usernameTaken"      },
                                 {ErrorCode::invalidCredentials,  "invalidCredentials" },
                                 {ErrorCode::weakPassword,        "weakPassword"       },
                                 {ErrorCode::usernameValidation,  "usernameValidation" },
                                 {ErrorCode::invalidToken,        "invalidToken"       },
                                 {ErrorCode::invalidRefreshToken, "invalidRefreshToken"},
                                 {ErrorCode::unknownMessageType,  "unknownMessageType" },
                                 {ErrorCode::notFound,            "notFound"           },
})
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

namespace protocol::ws
{
NLOHMANN_JSON_SERIALIZE_ENUM(WsMessageType,
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

namespace protocol
{
NLOHMANN_JSON_SERIALIZE_ENUM(RoomKind,
                             {
                                 {RoomKind::Direct,  "direct" },
                                 {RoomKind::Group,   "group"  },
                                 {RoomKind::Channel, "channel"},
})
NLOHMANN_JSON_SERIALIZE_ENUM(JoinPolicy,
                             {
                                 {JoinPolicy::Closed,   "closed"  },
                                 {JoinPolicy::ByMember, "byMember"},
                                 {JoinPolicy::ByAdmin,  "byAdmin" },
                                 {JoinPolicy::Public,   "public"  },
})
NLOHMANN_JSON_SERIALIZE_ENUM(WritePolicy,
                             {
                                 {WritePolicy::Everyone,   "everyone"  },
                                 {WritePolicy::AdminsOnly, "adminsOnly"},
})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomInfo, kind, joinPolicy, writePolicy)
} // namespace protocol
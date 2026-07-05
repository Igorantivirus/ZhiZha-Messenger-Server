#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <Protocol/Common/Parsing.hpp>
#include <Protocol/Common/Types.hpp>
#include <Protocol/Data/Rooms.hpp>
#include <Protocol/Data/Users.hpp>

namespace protocol::dto
{

// Ответ получение списка комнат
struct ListRoomsResponseDto
{
    std::vector<data::RoomWithLastMessage> rooms;
    std::unordered_map<UserId, data::UserDisplayInfo> postMessageSenders;
    bool hasMore;
};
PROTOCOL_JSON_SEREALIZE(ListRoomsResponseDto)

// Запрос содания комнаты
struct CreateRoomRequestDto
{
    std::string roomName;
    data::RoomInfo roomInfo;
    std::vector<UserId> invitedUsers;
};
PROTOCOL_JSON_SEREALIZE(CreateRoomRequestDto)
// Ответ создания комнаты
struct CreateRoomResponseDto
{
    RoomId roomId;
};
PROTOCOL_JSON_SEREALIZE(CreateRoomResponseDto)
// Запрос получения комнат по шаблону имени
struct LoopRoomsResponseDto
{
    std::unordered_map<RoomId, data::Room> rooms;
};
PROTOCOL_JSON_SEREALIZE(LoopRoomsResponseDto)

// Ответ комнаты по id
struct GetRoomResponseDto
{
    data::Room room;
};
PROTOCOL_JSON_SEREALIZE(GetRoomResponseDto)

// Запрос обновления настроек комнаты
struct UpdateRoomRequestDto
{
    std::string newName;
    data::RoomInfo newInfo;
};
PROTOCOL_JSON_SEREALIZE(UpdateRoomRequestDto)

// Ответ получения участников комнаты
struct ListMembersResponseDto
{
    std::vector<data::Member> members;
    bool hasMore;
};
PROTOCOL_JSON_SEREALIZE(ListMembersResponseDto)

// Запрос приглашения участника
struct InviteMemberRequestDto
{
    UserId invitedId;
};
PROTOCOL_JSON_SEREALIZE(InviteMemberRequestDto)

} // namespace protocol::dto
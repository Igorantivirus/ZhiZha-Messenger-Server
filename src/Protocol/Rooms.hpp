#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Types.hpp"
#include "Users.hpp"
#include "Messages.hpp"

// Спцеиальные данные
namespace protocol::rooms
{
// тип группы
enum class RoomKind
{
    Direct, // личные сообщения
    Group,  // группа
    Channel // канал
};
// правила присоединения
enum class JoinPolicy
{
    Closed,   // Никто не может добавлять и присоединяться
    ByMember, // участники могут приглашать
    ByAdmin,  // только админ может приглашать
    Public    // можно зайти любой
};
// Правила написания в группе
enum class WritePolicy
{
    Everyone,  // Все
    AdminsOnly // Только админы
};
// Информация о комнате
struct RoomInfo
{
    RoomKind kind;
    JoinPolicy joinPolicy;
    WritePolicy writePolicy;
};

// public информация о группе
struct Room
{
    RoomId id;
    std::string name;
    RoomInfo info;
};


struct RoomInformation
{
    Room roomInfo;
    messages::Message lastMessage;
};

// Информация об участнике
struct Member
{
    UserId userId;
};

} // namespace protocol::rooms

// клиент -> сервер
namespace protocol::rooms
{

struct CreateRoomRequest
{
    std::string roomName;
    RoomInfo roomInfo;
    std::vector<UserId> invitedUsers;
};

struct InviteMemberRequest
{
    UserId invitedId;
};

} // namespace protocol::rooms

// сервер -> клиент
namespace protocol::rooms
{

struct CreateRoomResponse
{
    RoomId roomId;
};

struct GetRoomsResponse
{
    std::vector<RoomInformation> rooms;
    std::unordered_map<UserId, users::UserDisplayInfo> postMessageSenders;
    bool hasMore;
};

struct RoomInfoResponse
{
    Room room;
};

struct RoomsMembersInfoResponse
{
    std::vector<Member> members;
};

} // namespace protocol::rooms
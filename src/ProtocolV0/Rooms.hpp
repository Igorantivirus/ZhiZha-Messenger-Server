#pragma once

#include <cstdint>
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
// Роль участника в комнате
enum class MemberRole
{
    Owner,  // создатель — полные права
    Admin,  // администратор
    Member  // обычный участник
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
    std::uint32_t participantsCount; // число участников комнаты
};

struct UsersRoomInfo
{
    RoomInformation roomInfo;
    MemberRole role;
};

// Информация об участнике. Несёт display-инфо (username+displayname), чтобы
// клиент отрисовал список участников без доп. запросов по каждому id.
struct Member
{
    UserId userId;
    users::UserDisplayInfo display;
    MemberRole role;
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

struct ChangeRoomRequest
{
    Room newRoomInfo;
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
    std::vector<UsersRoomInfo> rooms;
    std::unordered_map<UserId, users::UserDisplayInfo> postMessageSenders;
    bool hasMore;
};

struct RoomInfoResponse
{
    Room room;
};

struct RoomsLoopByExampleResponse
{
    std::unordered_map<RoomId, Room> users;
};

struct RoomsMembersInfoResponse
{
    std::vector<Member> members;
};

} // namespace protocol::rooms
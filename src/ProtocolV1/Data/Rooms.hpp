#pragma once

#include <cstdint>
#include <string>

#include <ProtocolV1/Common/Parsing.hpp>
#include <ProtocolV1/Common/Types.hpp>
#include <ProtocolV1/Data/Messages.hpp>
#include <ProtocolV1/Data/Users.hpp>

namespace protocol::data
{

// Тип комнаты.
enum class RoomKind : std::uint8_t
{
    Direct, // Личные сообщения (всегда 2 участника)
    Group,  // Обычная группа
    Channel // Канал
};
PROTOCOL_JSON_SEREALIZE(RoomKind)

// Кто и как может присоединяться к комнате
enum class JoinPolicy : std::uint8_t
{
    Closed,   // Никто не может приглашать и присоединяться
    ByMember, // Любой участник может приглашать
    ByAdmin,  // Только админ может приглашать
    Public    // Зайти может любой
};
PROTOCOL_JSON_SEREALIZE(JoinPolicy)

// Кто может писать в комнату
enum class WritePolicy : std::uint8_t
{
    Everyone,  // Все участники
    AdminsOnly // Только админы (для каналов и подобного)
};
PROTOCOL_JSON_SEREALIZE(WritePolicy)

// Роль участника в комнате.
enum class MemberRole : std::uint8_t
{
    Owner, // Создатель
    Admin, // Администратор
    Member // Обычный участник
};
PROTOCOL_JSON_SEREALIZE(MemberRole)

// Поведенческие настройки комнаты (без идентичности)
struct RoomInfo
{
    RoomKind kind;
    JoinPolicy joinPolicy;
    WritePolicy writePolicy;
};
PROTOCOL_JSON_SEREALIZE(RoomInfo)

// Публичная карточка комнаты.
struct Room
{
    RoomId id;
    std::string name;
    RoomInfo info;
};
PROTOCOL_JSON_SEREALIZE(Room)

// Комната в контексте конкретного пользователя
struct RoomWithLastMessage
{
    Room roomInfo;
    Message lastMessage;
    MemberRole senderRole;
    std::uint32_t participantsCount;
};
PROTOCOL_JSON_SEREALIZE(RoomWithLastMessage)

// Участник комнаты
struct Member
{
    UserId userId;
    UserDisplayInfo display;
    MemberRole role;
};
PROTOCOL_JSON_SEREALIZE(Member)

} // namespace protocol::data
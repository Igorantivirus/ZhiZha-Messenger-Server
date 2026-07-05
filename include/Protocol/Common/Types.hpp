#pragma once

#include <cstdint>

#include <Protocol/Common/Parsing.hpp>

namespace protocol
{
using UserId = std::int64_t;
using RoomId = std::int64_t;
using MessageId = std::int64_t;

// Страна
enum class Country : std::uint8_t
{
    None, // не указана
    Ru,   // Россия
    By,   // Беларусь
    Kz,   // Казахстан
    Ua,   // Украина
    Us,   // США
    Gb    // Великобритания
};
PROTOCOL_JSON_SEREALIZE(Country)

enum class RoomKind : std::uint8_t
{
    Direct, // Личные сообщения (всегда 2 участника)
    Group,  // Обычная группа
    Channel // Канал
};
PROTOCOL_JSON_SEREALIZE(RoomKind)

enum class JoinPolicy : std::uint8_t
{
    Closed,   // Никто не может приглашать и присоединяться
    ByMember, // Любой участник может приглашать
    ByAdmin,  // Только админ может приглашать
    Public    // Зайти может любой
};
PROTOCOL_JSON_SEREALIZE(JoinPolicy)

enum class WritePolicy : std::uint8_t
{
    Everyone,  // Все участники
    AdminsOnly // Только админы (для каналов и подобного)
};
PROTOCOL_JSON_SEREALIZE(WritePolicy)

enum class MemberRole : std::uint8_t
{
    Owner, // Создатель
    Admin, // Администратор
    Member // Обычный участник
};
PROTOCOL_JSON_SEREALIZE(MemberRole)

} // namespace protocol
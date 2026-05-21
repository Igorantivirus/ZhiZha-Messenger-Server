#pragma once

namespace protocol
{
// Тип комнаты
enum class RoomKind
{
    Direct,
    Group,
    Channel
};
// правила присоединения
enum class JoinPolicy
{
    Closed,
    ByMember,
    ByAdmin,
    Public
};
// Правила написания в группе
enum class WritePolicy
{
    Everyone,
    AdminsOnly
};

struct RoomInfo
{
    RoomKind kind;
    JoinPolicy joinPolicy;
    WritePolicy writePolicy;
};

// Для аждого конкретного пользователя права добавления
// и написания могут быть изменены правами от админа

} // namespace info
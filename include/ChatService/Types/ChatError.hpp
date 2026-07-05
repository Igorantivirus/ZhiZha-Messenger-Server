#pragma once

#include <cstdint>

namespace chat
{

enum class ChatError : std::uint8_t
{
    NotAMember,        // не состоит в комнате
    WriteForbidden,    // комната AdminsOnly, юзер не admin
    EmptyMessage,      // Пустое сообщение
    MessageTooLong,    // Сообщение солишком длинное
    RoomNotFound,      // Комната не найдена
    InvalidDirectRoom, // Ошибка создания лички
    InvalidRoomKind,   // Неверный тип комнаты
    EmptyRoomName,     // пустое имя комнаты
    MemberAlready,     // Уже участник
    PermissionError,   // Нельзя делать из-за ограничения разрешения
    UserNotFound       // Пользователь не существует
};

} // namespace chat
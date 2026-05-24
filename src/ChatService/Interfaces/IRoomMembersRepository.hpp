#pragma once

#include <ctime>
#include <optional>
#include <vector>

#include <Protocol/Types.hpp>

#include <ChatService/Types/MemberRole.hpp>
#include <ChatService/Types/RoomMember.hpp>

class IRoomMembersRepository
{
public:
    virtual ~IRoomMembersRepository() = default;

    // Добавить юзера в комнату с заданной ролью
    virtual void add(const protocol::RoomId roomId, const protocol::UserId userId, const MemberRole role, std::time_t joinedAt) = 0;

    // Убрать юзера из комнаты
    virtual void remove(const protocol::RoomId roomId, const protocol::UserId userId) = 0;

    // Список всех участников комнаты (нужно для broadcast'а сообщения)
    virtual std::vector<RoomMember> membersOf(const protocol::RoomId roomId) const = 0;

    // Проверка членства (для авторизации действий — может ли он писать сюда?)
    virtual bool isMember(const protocol::RoomId roomId, protocol::UserId userId) const = 0;

    // Получить запись об участнике (с ролью и lastRead) — для проверок прав
    virtual std::optional<RoomMember> get(const protocol::RoomId roomId, const protocol::UserId userId) const = 0;

    // Обновить позицию чтения (read receipts)
    virtual void updateLastRead(const protocol::RoomId roomId, const protocol::UserId userId, const protocol::MessageId lastReadMessageId) = 0;
};
#pragma once

#include <vector>
#include <optional>
#include <ctime>

#include <Utils/Types.hpp>

#include <ChatService/Types/MemberRole.hpp>
#include <ChatService/Types/RoomMember.hpp>

class IRoomMembersRepository
{
public:
    virtual ~IRoomMembersRepository() = default;

    // Добавить юзера в комнату с заданной ролью
    virtual void add(const RoomId roomId, const UserId userId, const MemberRole role, std::time_t joinedAt) = 0;

    // Убрать юзера из комнаты
    virtual void remove(const RoomId roomId, const UserId userId) = 0;

    // Список всех участников комнаты (нужно для broadcast'а сообщения)
    virtual std::vector<RoomMember> membersOf(const RoomId roomId) const = 0;

    // Проверка членства (для авторизации действий — может ли он писать сюда?)
    virtual bool isMember(const RoomId roomId, UserId userId) const = 0;

    // Получить запись об участнике (с ролью и lastRead) — для проверок прав
    virtual std::optional<RoomMember> get(const RoomId roomId, const UserId userId) const = 0;

    // Обновить позицию чтения (read receipts)
    virtual void updateLastRead(const RoomId roomId, const UserId userId, const MessageId lastReadMessageId) = 0;
};
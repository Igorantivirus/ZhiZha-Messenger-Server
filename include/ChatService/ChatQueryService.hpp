#pragma once

#include <expected>
#include <string>
#include <vector>

#include <Utils/Types.hpp>

#include "Subservices/MembersRepository.hpp"
#include "Subservices/MessageRepository.hpp"
#include "Subservices/RoomRepository.hpp"
#include "Types/ChatError.hpp"
#include "Types/Member.hpp"
#include "Types/Message.hpp"
#include "Types/Room.hpp"
#include "Types/RoomForUser.hpp"

namespace chat
{

// Read-only сервис чата. Возвращает доменные структуры chat::* — в них нет
// секретов, отдельные View не заводим (введём, если появятся приватные поля).
//
// Display-имена пользователей этот сервис НЕ собирает — кросс-доменные view
// (комната + автор последнего сообщения, участник + displayName) клеит
// транспорт через UserQueryService. Так chat-модуль не знает про auth.
//
// Сосед ChatService про этот сервис не знает; оба держат одни и те же
// репозитории — ChatQueryService держит их по const&.
class ChatQueryService
{
public:
    template <typename ElemType>
    struct Page
    {
        std::vector<ElemType> elems;
        bool hasMore;
    };

public:

    ChatQueryService
    (
        const RoomRepository &rooms,
        const MessageRepository &messages,
        const MembersRepository &members
    );

    // Получить карточку комнаты. Проверяет, что requester состоит в комнате
    // (PermissionError, если нет; RoomNotFound, если такой комнаты нет).
    std::expected<Room, ChatError> getRoom
    (
        utils::UserId requesterId,
        utils::RoomId roomId
    ) const;

    // Страница комнат пользователя «вперёд» от afterId (0 = с начала).
    // Возвращает RoomForUser (room + lastMessage + role + participantsCount).
    std::expected<Page<RoomForUser>, ChatError> getRoomsOfUser
    (
        utils::UserId userId,
        utils::RoomId afterId,
        unsigned limit
    ) const;

    // Страница сообщений комнаты «вперёд» от afterId (0 = с начала).
    // Проверяет членство requester в комнате.
    std::expected<Page<Message>, ChatError> getMessages
    (
        utils::UserId requesterId,
        utils::RoomId roomId,
        std::optional<utils::MessageId> afterId,
        std::optional<utils::MessageId> beforeId,
        unsigned limit
    ) const;

    // Страница участников комнаты «вперёд» от afterId (0 = с начала).
    // Проверяет членство requester в комнате.
    std::expected<Page<Member>, ChatError> getMembers
    (
        utils::UserId requesterId,
        utils::RoomId roomId,
        utils::UserId afterId,
        unsigned limit
    ) const;

    // Поиск публичных комнат по подстроке имени для UI-выбора.
    // Фильтрация по joinPolicy = Public выполняется на уровне репозитория —
    // приватные комнаты не утекают в выдачу даже при опечатке в контроллере.
    std::vector<Room> searchRooms(const std::string &query, unsigned limit) const;

private:
    const RoomRepository &rooms_;
    const MessageRepository &messages_;
    const MembersRepository &members_;
};

} // namespace chat

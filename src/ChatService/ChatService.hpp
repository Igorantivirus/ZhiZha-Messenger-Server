#pragma once

#include <expected>

#include <Utils/Time.hpp>
#include <Utils/Types.hpp>

#include <Protocol/Ws.hpp>

#include <ChatService/Interfaces/IMessageRepository.hpp>
#include <ChatService/Interfaces/IRoomMembersRepository.hpp>
#include <ChatService/Interfaces/IRoomRepository.hpp>

#include <Sessions/SessionManager.hpp>

#include "Types/ChatError.hpp"
#include "Types/MemberRole.hpp"
#include "Types/RoomMember.hpp"
#include "nlohmann/json.hpp"

class ChatService
{
public:
    ChatService(IRoomRepository &roomRepo, IMessageRepository &messageRepo, IRoomMembersRepository &roomMembersRepo, SessionManager &sessions)
        : roomRepo_(roomRepo),
          messageRepo_(messageRepo),
          roomMembersRepo_(roomMembersRepo),
          sessions_(sessions)
    {
    }

    std::expected<MessageId, ChatError> messageFromUser(
        const UserId sender,
        const protocol::ws::SendMessageRequest &request)
    {
        // 1. Валидация текста
        if (request.text.empty())
            return std::unexpected(ChatError::EmptyMessage);
        if (request.text.size() > 1000)
            return std::unexpected(ChatError::MessageTooLong);

        // 2. Достаём комнату — нужна для проверки writePolicy
        auto room = roomRepo_.findById(request.roomId);
        if (!room)
            return std::unexpected(ChatError::RoomNotFound);

        // 3. Достаём запись участника (одним запросом получаем и факт членства, и роль)
        auto member = roomMembersRepo_.get(request.roomId, sender);
        if (!member)
            return std::unexpected(ChatError::NotAMember);

        // 4. Проверка прав на запись
        if (room->info.writePolicy == info::WritePolicy::AdminsOnly && member->role == MemberRole::Member)
            return std::unexpected(ChatError::WriteForbidden);

        // 5. Сохраняем
        const MessageId msgId = messageRepo_.create(request.roomId, sender, request.text);

        const std::time_t now = getCurrentTime();

        // 6. Формируем event для рассылки (один раз — потом разошлём всем)
        Message msg{
            .id = msgId,
            .roomId = request.roomId,
            .fromUserId = sender,
            .text = request.text,
            .createdAt = now};

        protocol::ws::NewMessageEvent event{
            .messageId = msgId,
            .roomId = request.roomId,
            .senderId = sender,
            .text = request.text,
            .createdAt = now};

        const std::string payload = nlohmann::json(event).dump();

        // 7. Рассылаем участникам комнаты, кто онлайн
        auto members = roomMembersRepo_.membersOf(request.roomId);
        for (const auto &m : members)
            sessions_.sendToUser(m.userId, payload);

        return msgId;
    }

    std::expected<RoomId, ChatError> createRoom(
        const UserId sender,
        const protocol::ws::CreateRoomRequest &request)
    {
        // Валидация для Direct
        if (request.roomInfo.kind == info::RoomKind::Direct)
        {
            if (request.invitedUsers.size() != 1)
                return std::unexpected(ChatError::InvalidDirectRoom);
            // Direct — это всегда 2 человека: создатель + 1 приглашённый
        }

        // Валидация имени (для Direct можно пустое)
        if (request.roomInfo.kind != info::RoomKind::Direct && request.roomName.empty())
            return std::unexpected(ChatError::EmptyRoomName);

        const std::time_t now = getCurrentTime();
        const RoomId roomId = roomRepo_.create(request.roomName, request.roomInfo, now);

        roomMembersRepo_.add(roomId, sender, MemberRole::Owner, now);
        for (const UserId invitedId : request.invitedUsers)
        {
            if (invitedId == sender)
                continue; // не добавляем себя дважды
            roomMembersRepo_.add(roomId, invitedId, MemberRole::Member, now);
        }

        // Уведомить всех приглашённых, кто онлайн
        protocol::ws::RoomCreatedEvent event{.roomId = roomId,
                                             /* + room info */};
        const std::string payload = nlohmann::json(event).dump();
        for (const UserId invitedId : request.invitedUsers)
            sessions_.sendToUser(invitedId, payload);

        return roomId;
    }

    void leaveRoom(const UserId sender, const protocol::ws::LeaveRoomRequest &request)
    {
        // Проверяем, что юзер вообще состоял в комнате
        auto member = roomMembersRepo_.get(request.roomId, sender);
        if (!member)
            return; // и без того не в комнате — нечего делать

        // Особая обработка для Owner'а
        if (member->role == MemberRole::Owner)
        {
            // Политика: запретить owner'у выходить, не передав права.
            // Для прототипа — можно сделать выход без передачи, но тогда комната станет "сиротской".
            // Я бы передал права на самого старого admin'а, либо запретил.
            // Здесь — пример с передачей или удалением комнаты.

            auto members = roomMembersRepo_.membersOf(request.roomId);
            // Ищем кого-нибудь, кроме owner'а
            std::optional<UserId> newOwner;
            for (const auto &m : members)
            {
                if (m.userId != sender)
                {
                    newOwner = m.userId;
                    break;
                }
            }

            if (!newOwner)
            {
                // Owner один в комнате — просто удаляем комнату
                roomRepo_.remove(request.roomId); // CASCADE уберёт всё остальное
                return;
            }

            // Передаём права (если у тебя есть updateRole — используй его)
            // TODO:
            // roomMembersRepo_.updateRole(request.roomId, *newOwner, MemberRole::Owner);
        }

        // Убираем самого юзера
        roomMembersRepo_.remove(request.roomId, sender);

        // Уведомляем остальных
        protocol::ws::UserLeftEvent event{
            .roomId = request.roomId,
            .userId = sender};
        const std::string payload = nlohmann::json(event).dump();

        auto remaining = roomMembersRepo_.membersOf(request.roomId);
        if (remaining.empty())
        {
            // Никого не осталось — удаляем комнату полностью
            roomRepo_.remove(request.roomId); // CASCADE подтянет messages
            return;
        }
        for (const auto &m : remaining)
            sessions_.sendToUser(m.userId, payload);
    }

private:
    IRoomRepository &roomRepo_;
    IMessageRepository &messageRepo_;
    IRoomMembersRepository &roomMembersRepo_;

    SessionManager &sessions_;
};
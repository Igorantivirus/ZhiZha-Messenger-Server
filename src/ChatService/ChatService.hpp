#pragma once

#include <expected>

#include <crow/crow.h>

#include <Utils/Time.hpp>

#include <Protocol/Parsing.hpp>
#include <Protocol/Rooms.hpp>
#include <Protocol/Types.hpp>
#include <Protocol/Ws.hpp>

#include <ChatService/Interfaces/IMessageRepository.hpp>
#include <ChatService/Interfaces/IRoomMembersRepository.hpp>
#include <ChatService/Interfaces/IRoomRepository.hpp>

#include <Sessions/SessionManager.hpp>
#include <optional>
#include <ranges>

#include "Types/ChatError.hpp"
#include "Types/MemberRole.hpp"
#include "Types/MessagePage.hpp"
#include "Types/Room.hpp"
#include "Types/RoomMember.hpp"
#include "Types/RoomWithLastMessage.hpp"

class ChatService
{
public:
    ChatService(IRoomRepository &roomRepo,
                IMessageRepository &messageRepo,
                IRoomMembersRepository &roomMembersRepo,
                SessionManager &sessions,
                const std::size_t maxMessageSize)
        : roomRepo_(roomRepo),
          messageRepo_(messageRepo),
          roomMembersRepo_(roomMembersRepo),
          sessions_(sessions),
          maxMessageSize_(maxMessageSize)
    {
    }

public: // Actions without answers
    void onUserConnected(protocol::UserId userId)
    {
    }
    void onUserDisconnected(protocol::UserId userId)
    {
    }

    std::expected<protocol::MessageId, ChatError> onSendMessage(const protocol::UserId sender, crow::websocket::connection &senderConn, const protocol::ws::SendMessageRequest &request)
    {
        // 1. Валидация текста
        if (request.text.empty())
            return std::unexpected(ChatError::EmptyMessage);
        if (request.text.size() > maxMessageSize_)
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

        if (room->info.writePolicy == protocol::rooms::WritePolicy::AdminsOnly && member->role == MemberRole::Member)
            return std::unexpected(ChatError::WriteForbidden);

        // 5. Сохраняем. create() возвращает готовое сообщение с присвоенным id
        //    и серверным createdAt — это единственный источник времени, его же
        //    шлём и в ack, и в рассылке (никаких повторных замеров).
        const Message msg = messageRepo_.create(request.roomId, sender, request.text);

        // 6. СНАЧАЛА подтверждаем — но только тому соединению, что прислало
        //    сообщение. Ack несёт usersMessageId, по которому именно это
        //    соединение свяжет свой временный id с присвоенным сервером.
        protocol::ws::MessageAckEvent ack{
            .usersMessageId = request.usersMessageId,
            .messageId = msg.id,
            .roomId = msg.roomId,
            .createdAt = msg.createdAt};
        sessions_.sendToConnection(senderConn, nlohmann::json(ack).dump());

        // 7. Рассылаем NewMessage всем участникам, КРОМE того одного
        //    соединения, что прислало сообщение (оно получило Ack). Другие
        //    устройства отправителя получают NewMessage как обычные участники
        //    и по senderId == свой userId понимают «это моё сообщение».
        protocol::ws::NewMessageEvent event{
            .messageId = msg.id,
            .roomId = msg.roomId,
            .senderId = msg.fromUserId,
            .text = msg.text,
            .createdAt = msg.createdAt};
        const std::string payload = nlohmann::json(event).dump();

        for (const auto &m : roomMembersRepo_.membersOf(request.roomId))
            sessions_.sendToUserExcept(m.userId, &senderConn, payload);

        return msg.id;
    }

public: // Actions with responses
    std::expected<MessagePage, ChatError> getMessages(
        const protocol::UserId sender,
        const protocol::RoomId roomId,
        std::optional<const protocol::MessageId> beforeId,
        std::optional<const protocol::MessageId> afterId,
        unsigned limit)
    {
        if (!roomMembersRepo_.isMember(roomId, sender))
            return std::unexpected(ChatError::NotAMember);

        if (limit == 0)
            limit = 50;
        if (limit > 100)
            limit = 100;

        if (afterId)
        {
            auto candidates = messageRepo_.findAfter(roomId, *afterId, limit + 1);

            if (candidates.size() > limit)
            {
                // Разрыв слишком большой. Возвращаем свежайшие limit.
                auto latest = messageRepo_.findLatest(roomId, limit);
                std::reverse(latest.begin(), latest.end()); // DESC → ASC
                return MessagePage{
                    .messages = std::move(latest),
                    .hasMore = true // клиент знает: между afterId и первым из этих — пропуск
                };
            }
            return MessagePage{.messages = std::move(candidates), .hasMore = false};
        }
        if (beforeId)
        {
            // limit + 1 чтобы знать, есть ли ещё раньше
            auto messages = messageRepo_.findBefore(roomId, *beforeId, limit + 1);
            bool hasMore = false;
            if (messages.size() > limit)
            {
                hasMore = true;
                messages.pop_back(); // выкинуть лишний, он был только для определения hasMore
            }
            // findBefore возвращает DESC — переворачиваем в ASC для клиента
            std::reverse(messages.begin(), messages.end());
            // Есть ещё старше
            return MessagePage{.messages = std::move(messages), .hasMore = hasMore};
        }
        // Последние limit сообщений
        auto messages = messageRepo_.findLatest(roomId, limit + 1);

        bool hasMore = false;
        if (messages.size() > limit)
        {
            hasMore = true;
            messages.pop_back();
        }
        // findLatest возвращает DESC — переворачиваем
        std::reverse(messages.begin(), messages.end());
        return MessagePage{.messages = std::move(messages), .hasMore = hasMore};
    }

    std::expected<std::vector<RoomMember>, ChatError> getMembers(const protocol::UserId userId, const protocol::RoomId roomId)
    {
        if (!roomMembersRepo_.isMember(roomId, userId))
            return std::unexpected(ChatError::NotAMember);
        return roomMembersRepo_.membersOf(roomId);
    }

    std::optional<ChatError> inviteUser(const protocol::UserId inviter, const protocol::RoomId roomId, const protocol::UserId invited)
    {
        // Вместо трёх проверок, что участник, что есть комната и получение пользователя - достаточно две
        auto room = roomRepo_.findById(roomId);
        if (!room)
            return ChatError::RoomNotFound;
        auto member = roomMembersRepo_.get(roomId, inviter);
        if (!member)
            return ChatError::NotAMember;

        if // проверка на возможность приглашения
            (
                room->info.joinPolicy == protocol::rooms::JoinPolicy::Closed ||                                                                             // Если нельзя приглашать
                (room->info.joinPolicy == protocol::rooms::JoinPolicy::ByAdmin && (member->role != MemberRole::Admin && member->role != MemberRole::Owner)) // Если приглашать могут только админы, а приглашающий - не админ и не создатель
            )
            return ChatError::PermissionError;

        if (roomMembersRepo_.isMember(roomId, invited))
            return ChatError::MemberAlready;

        // Все проверки пройдены — фактически добавляем участника.
        roomMembersRepo_.add(roomId, invited, MemberRole::Member, getCurrentTime());

        // Уведомляем участников комнаты (включая только что добавленного,
        // его клиент так узнает, что комната ему доступна). membersOf уже
        // содержит invited, так как add выше прошёл.
        protocol::ws::UserJoinedEvent event{.roomId = roomId, .userId = invited};
        const std::string payload = nlohmann::json(event).dump();
        for (const auto &m : roomMembersRepo_.membersOf(roomId))
            sessions_.sendToUser(m.userId, payload);

        return std::nullopt;
    }

    std::expected<Room, ChatError> getRoomInfo(const protocol::UserId userId, const protocol::RoomId roomId)
    {
        if (!roomMembersRepo_.isMember(roomId, userId))
            return std::unexpected(ChatError::NotAMember);
        auto room = roomRepo_.findById(roomId);
        if (!room)
            return std::unexpected(ChatError::RoomNotFound);
        return *room;
    }

    std::expected<std::vector<RoomWithLastMessage>, ChatError> getRoomsByUser(const protocol::UserId userId, const unsigned limit, std::optional<protocol::RoomId> lastLoadedId)
    {
        auto rooms = roomRepo_.findForUser(userId, limit, lastLoadedId);

        return rooms | std::views::transform([this](Room &room) -> RoomWithLastMessage
        {
            // Читаем id ДО перемещения room — иначе обращение к moved-from объекту.
            const protocol::RoomId roomId = room.id;

            RoomWithLastMessage res;
            res.room = std::move(room);
            auto msg = messageRepo_.findLastMessageInRoom(roomId);
            if (msg)
                res.msg = std::move(msg.value());
            return res;
        }) | std::ranges::to<std::vector<RoomWithLastMessage>>();
    }

    std::expected<protocol::RoomId, ChatError> createRoom(const protocol::UserId sender, const protocol::rooms::CreateRoomRequest &request)
    {
        // Валидация для Direct
        if (request.roomInfo.kind == protocol::rooms::RoomKind::Direct)
        {
            if (request.invitedUsers.size() != 1)
                return std::unexpected(ChatError::InvalidDirectRoom);
            // Direct — это всегда 2 человека: создатель + 1 приглашённый
        }

        // Валидация имени (для Direct можно пустое)
        if (request.roomInfo.kind != protocol::rooms::RoomKind::Direct && request.roomName.empty())
            return std::unexpected(ChatError::EmptyRoomName);

        const std::time_t now = getCurrentTime();
        const protocol::RoomId roomId = roomRepo_.create(request.roomName, request.roomInfo, now);

        roomMembersRepo_.add(roomId, sender, MemberRole::Owner, now);
        for (const protocol::UserId invitedId : request.invitedUsers)
        {
            if (invitedId == sender)
                continue; // не добавляем себя дважды
            roomMembersRepo_.add(roomId, invitedId, MemberRole::Member, now);
        }

        // Уведомить всех участников комнаты, кто онлайн — включая создателя.
        // Создателю событие нужно для мультидевайса: другие его устройства
        // (не те, что слали HTTP-запрос) так узнают о новой комнате. Несём
        // полную Room, чтобы клиент отрисовал её в списке без доп. запроса.
        protocol::ws::RoomCreatedEvent event{
            .room = protocol::rooms::Room{
                .id = roomId,
                .name = request.roomName,
                .info = request.roomInfo}};
        const std::string payload = nlohmann::json(event).dump();
        for (const auto &m : roomMembersRepo_.membersOf(roomId))
            sessions_.sendToUser(m.userId, payload);

        return roomId;
    }

    std::optional<ChatError> leaveRoom(const protocol::UserId sender, const protocol::RoomId roomId)
    {
        // Проверяем, что юзер вообще состоял в комнате
        auto member = roomMembersRepo_.get(roomId, sender);
        if (!member)
            return ChatError::NotAMember; // и без того не в комнате — нечего делать

        // Особая обработка для Owner'а
        if (member->role == MemberRole::Owner)
        {
            // Политика: запретить owner'у выходить, не передав права.
            // Для прототипа — можно сделать выход без передачи, но тогда комната станет "сиротской".
            // Я бы передал права на самого старого admin'а, либо запретил.
            // Здесь — пример с передачей или удалением комнаты.

            auto members = roomMembersRepo_.membersOf(roomId);
            // Ищем кого-нибудь, кроме owner'а
            std::optional<protocol::UserId> newOwner;
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
                roomRepo_.remove(roomId); // CASCADE уберёт всё остальное
                return std::nullopt;
            }

            // Передаём права
            // TODO:
            // roomMembersRepo_.updateRole(request.roomId, *newOwner, MemberRole::Owner);
        }

        // Убираем самого юзера
        roomMembersRepo_.remove(roomId, sender);

        // Уведомляем остальных
        protocol::ws::UserLeftEvent event{
            .roomId = roomId,
            .userId = sender};
        const std::string payload = nlohmann::json(event).dump();

        auto remaining = roomMembersRepo_.membersOf(roomId);
        if (remaining.empty())
        {
            // Никого не осталось — удаляем комнату полностью
            roomRepo_.remove(roomId); // CASCADE подтянет messages
            return std::nullopt;
        }
        for (const auto &m : remaining)
            sessions_.sendToUser(m.userId, payload);
        return std::nullopt;
    }

private:
    IRoomRepository &roomRepo_;
    IMessageRepository &messageRepo_;
    IRoomMembersRepository &roomMembersRepo_;

    SessionManager &sessions_;

    const std::size_t maxMessageSize_;
};
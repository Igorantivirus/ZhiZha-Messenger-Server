#pragma once

#include <expected>
#include <string>
#include <vector>

#include <Utils/Types.hpp>

#include "Subservices/MembersRepository.hpp"
#include "Subservices/MessageRepository.hpp"
#include "Subservices/RoomRepository.hpp"
#include "Types/ChatError.hpp"

namespace chat
{

// Command-сервис чата. Только мутации с проверкой прав и побочными эффектами.
// Чтения (комната, страницы комнат/сообщений/участников) — в ChatQueryService,
// он работает напрямую с теми же репозиториями.
//
// Notifiables — список пользователей, которым транспорт должен разослать
// WS-событие об изменении (например, всем участникам комнаты при апдейте).
// В список включён и инициатор — транспорт сам решает, слать ли ему
// (у инициатора обычно есть HTTP-ответ). См. решение #5.
class ChatService
{
public:
    using Notifiables = std::vector<utils::UserId>;

    struct SendMessageResponse
    {
        utils::MessageId messageId;
        Notifiables notifiables;
        std::time_t createdAt;
    };

    ChatService(RoomRepository &rooms,
                MessageRepository &messages,
                MembersRepository &members);

public: // operations with room
    std::expected<utils::RoomId, ChatError> createRoom(
        utils::UserId creator,
        std::string name,
        utils::RoomInfo info,
        std::vector<utils::UserId> invitedUsers);

    std::expected<Notifiables, ChatError> deleteRoom(
        utils::UserId userId,
        utils::RoomId roomId);

    std::expected<Notifiables, ChatError> updateRoom(
        utils::UserId userId,
        utils::RoomId roomId,
        utils::RoomInfo info,
        std::string roomName);

public: // operations with members
    std::expected<Notifiables, ChatError> inviteToRoom(
        utils::UserId invitorId,
        utils::UserId invitedId,
        utils::RoomId roomId);

    std::expected<Notifiables, ChatError> joinToRoom(
        utils::UserId userId,
        utils::RoomId roomId);

    std::expected<Notifiables, ChatError> kickFromRoom(
        utils::UserId kickerId,
        utils::UserId kickedId,
        utils::RoomId roomId);

    std::expected<Notifiables, ChatError> leaveFromRoom(
        utils::UserId userId,
        utils::RoomId roomId);

public: // operations with messages
    std::expected<SendMessageResponse, ChatError> sendMessage(
        utils::UserId userId,
        utils::RoomId roomId,
        std::string message);

private:
    RoomRepository &rooms_;
    MessageRepository &messages_;
    MembersRepository &members_;
};

} // namespace chat

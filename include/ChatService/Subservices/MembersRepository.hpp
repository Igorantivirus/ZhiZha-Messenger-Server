#pragma once

#include <optional>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Utils/Types.hpp>

#include <ChatService/Types/Member.hpp>

namespace chat
{

// Репозиторий членства "пользователь ↔ комната". Знает только таблицу roomMembers.
// Не лезет в rooms/messages — для Room используем RoomRepository::findByIds(ids).
class MembersRepository
{
public:
    explicit MembersRepository(SQLite::Database &db);

    // joinedAt ставит сам репозиторий.
    Member create(const utils::RoomId roomId, const utils::UserId userId, const utils::MemberRole role);

    std::optional<Member> findByIds(const utils::RoomId roomId, const utils::UserId userId) const;
    void removeById(const utils::RoomId roomId, const utils::UserId userId);
    void removeAllInRoom(const utils::RoomId roomId);

    void updateRole(const utils::RoomId roomId, const utils::UserId userId, const utils::MemberRole role);
    void updateLastMessageId(const utils::RoomId roomId, const utils::UserId userId, const utils::MessageId msgId);

    // Пагинация: ids комнат, в которых состоит пользователь, после afterId.
    // ChatService потом запросит RoomRepository::findByIds(ids) — два запроса
    // вместо JOIN, каждый репо знает только свою таблицу.
    std::vector<utils::RoomId> findRoomIdsForUserAfter(const utils::UserId userId,
                                                      const unsigned limit,
                                                      const utils::RoomId afterId) const;

    // Пагинация участников комнаты для UI: полные Member.
    std::vector<Member> findMembersAfter(const utils::RoomId roomId,
                                         const unsigned limit,
                                         const utils::UserId afterId) const;

    // Только ids участников комнаты — для рассылки WS-событий.
    std::vector<utils::UserId> findMemberIdsInRoom(const utils::RoomId roomId) const;

    // Все ids комнат пользователя (без пагинации) — например, для broadcastUserUpdated.
    std::vector<utils::RoomId> roomIdsOfUser(const utils::UserId userId) const;

    unsigned findCountMembersById(const utils::RoomId roomId) const;
    bool isMember(const utils::RoomId roomId, const utils::UserId userId) const;

private:
    SQLite::Database &db_;
};

} // namespace chat

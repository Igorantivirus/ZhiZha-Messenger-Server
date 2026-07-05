#pragma once

#include <optional>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Utils/Types.hpp>

#include <ChatService/Types/Message.hpp>

namespace chat
{

// Репозиторий сообщений. Знает только таблицу messages.
class MessageRepository
{
public:
    explicit MessageRepository(SQLite::Database &db);

    // createdAt ставит сам репозиторий.
    Message create(const utils::RoomId roomId, const utils::UserId fromUserId, const std::string &text);

    std::optional<Message> findById(const utils::MessageId id) const;
    void removeAllInRoom(const utils::RoomId roomId);

    std::optional<Message> findLastMessageInRoom(const utils::RoomId roomId) const;

    // Курсорная пагинация. afterId/beforeId — id-границы, limit — лимит выборки.
    std::vector<Message> findAfter(const utils::RoomId roomId, const unsigned limit, const utils::MessageId afterId) const;
    std::vector<Message> findBefore(const utils::RoomId roomId, const unsigned limit, const utils::MessageId beforeId) const;
    std::vector<Message> findLatest(const utils::RoomId roomId, const unsigned limit) const;

private:
    SQLite::Database &db_;
};

} // namespace chat

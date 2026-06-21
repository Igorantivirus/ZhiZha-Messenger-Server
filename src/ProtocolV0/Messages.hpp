#pragma once

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

#include "Types.hpp"
#include "Users.hpp"

namespace protocol::messages
{
struct Message
{
    MessageId id;
    UserId fromUserId;
    std::string text;
    std::time_t createdAt;
};

struct MessagesResponse
{
    RoomId roomId;
    std::vector<Message> messages;
    bool hasMore;
    // Public-инфо авторов сообщений этой страницы. Заполняется только если
    // клиент запросил ?withSenders=true; иначе остаётся пустым (структура
    // ответа неизменна — one endpoint → one structure).
    std::unordered_map<UserId, users::UserDisplayInfo> senders;
};

} // namespace protocol::messages
#pragma once

#include <optional>
#include <vector>

#include <ChatService/Types/Message.hpp>

class IMessageRepository
{
public:
    virtual ~IMessageRepository() = default;
    virtual MessageId create(const RoomId roomId, const UserId fromUserId, const std::string &text) = 0;
    virtual std::optional<Message> findById(const MessageId id) const = 0;
    virtual std::vector<Message> findAfter(const RoomId roomId, const MessageId afterId, const unsigned limit) const = 0;
    virtual std::vector<Message> findBefore(const RoomId roomId, const MessageId beforeId, const unsigned limit) const = 0;
    virtual void removeAllInRoom(const RoomId roomId) = 0;
};
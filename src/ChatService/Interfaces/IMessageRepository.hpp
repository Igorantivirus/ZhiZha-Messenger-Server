#pragma once

#include <optional>
#include <vector>

#include <Protocol/Types.hpp>

#include <ChatService/Types/Message.hpp>

class IMessageRepository
{
public:
    virtual ~IMessageRepository() = default;
    virtual protocol::MessageId create(const protocol::RoomId roomId, const protocol::UserId fromUserId, const std::string &text) = 0;
    virtual std::optional<Message> findById(const protocol::MessageId id) const = 0;
    virtual std::vector<Message> findAfter(const protocol::RoomId roomId, const protocol::MessageId afterId, const unsigned limit) const = 0;
    virtual std::vector<Message> findBefore(const protocol::RoomId roomId, const protocol::MessageId beforeId, const unsigned limit) const = 0;
    virtual std::vector<Message> findLatest(const protocol::RoomId roomId, const unsigned limit) const = 0;
    virtual void removeAllInRoom(const protocol::RoomId roomId) = 0;
};
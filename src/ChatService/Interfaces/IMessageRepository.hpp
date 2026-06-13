#pragma once

#include <optional>
#include <vector>

#include <Protocol/Types.hpp>

#include <ChatService/Types/Message.hpp>

class IMessageRepository
{
public:
    virtual ~IMessageRepository() = default;
    // Создаёт сообщение и возвращает его целиком — с присвоенным id и
    // серверным createdAt. Это единственный источник времени создания:
    // вызывающий не должен замерять время отдельно, иначе оно разойдётся
    // с тем, что записано в БД.
    virtual Message create(const protocol::RoomId roomId, const protocol::UserId fromUserId, const std::string &text) = 0;
    virtual std::optional<Message> findById(const protocol::MessageId id) const = 0;
    virtual std::vector<Message> findAfter(const protocol::RoomId roomId, const protocol::MessageId afterId, const unsigned limit) const = 0;
    virtual std::vector<Message> findBefore(const protocol::RoomId roomId, const protocol::MessageId beforeId, const unsigned limit) const = 0;
    virtual std::vector<Message> findLatest(const protocol::RoomId roomId, const unsigned limit) const = 0;
    virtual std::optional<Message> findLastMessageInRoom(const protocol::RoomId roomId) const = 0;
    virtual void removeAllInRoom(const protocol::RoomId roomId) = 0;
};
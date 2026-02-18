#include "core/Room.hpp"
#include "Types.hpp"

Room::Room(IDType roomId, Type type, const std::string& name) : roomId_(roomId), type_(type), name_(name)
{
}

void Room::broadcast(const std::string& message) const
{
    for (const auto& user : users_)
    {
        if (user != nullptr && user->connection != nullptr)
        {
            user->connection->send_text(message);
        }
    }
}

void Room::setName(const std::string& name)
{
    name_ = name;
}
const std::string& Room::getName() const
{
    return name_;
}

void Room::addUser(const UserContextPtr& user)
{
    users_.insert(user);
}

void Room::removeUser(const UserContextPtr& user)
{
    users_.erase(user);
}

bool Room::hasUser(const UserContextPtr& user) const
{
    return users_.contains(user);
}

bool Room::empty() const
{
    return users_.empty();
}

IDType Room::id() const
{
    return roomId_;
}

Room::Type Room::type() const
{
    return type_;
}


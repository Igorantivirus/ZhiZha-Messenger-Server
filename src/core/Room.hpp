#pragma once

#include <memory>
#include <set>
#include <string>

#include "core/Types.hpp"
#include "core/UserContext.hpp"

using UserContextPtr = std::shared_ptr<UserContext>;

class Room
{
public:
    enum class Type : bool
    {
        Private,
        Public
    };

    Room() = default;
    Room(IDType roomId, Type type, const std::string& name);

    void broadcast(const std::string& message) const;
    void addUser(const UserContextPtr& user);
    void removeUser(const UserContextPtr& user);
    
    void setName(const std::string& name);
    const std::string& getName() const;

    [[nodiscard]] bool hasUser(const UserContextPtr& user) const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] IDType id() const;
    [[nodiscard]] Type type() const;

private:
    IDType roomId_ = 0;
    std::string name_;
    Type type_ = Type::Public;
    std::set<UserContextPtr> users_;
};


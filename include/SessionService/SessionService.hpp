#pragma once

#include "Types/Session.hpp"

namespace session
{

class SessionService
{
public:
    void add(crow::websocket::connection &conn, utils::UserId userId);
    void remove(crow::websocket::connection &conn);
    std::optional<utils::UserId> userIdOf(crow::websocket::connection &conn) const;

    void sendToConnection(crow::websocket::connection &conn, const std::string& message) const;

    void sendToUser(const utils::UserId userId, const std::string& message) const;
    void sendToUserExcept(const utils::UserId userId, const crow::websocket::connection &except, const std::string& message) const;
    
    void sendToUsers(const std::span<utils::UserId> usersId, const std::string& message) const;
    void sendToUsersExcept(const std::span<utils::UserId> usersId, const crow::websocket::connection &except, const std::string& message) const;

    bool isOnline(utils::UserId userId) const;

private:
    mutable std::shared_mutex mtx_;
    std::unordered_map<crow::websocket::connection *, Session> byConnection_;
    std::unordered_map<utils::UserId, std::unordered_set<crow::websocket::connection *>> byUser_;
};

} // namespace session
#include <SessionService/SessionService.hpp>

namespace session
{

void SessionService::add(crow::websocket::connection &conn, utils::UserId userId)
{
    std::unique_lock lock(mtx_);
    byConnection_.insert_or_assign(&conn, Session{.userId = userId, .connection = &conn});
    byUser_[userId].insert(&conn);
}
void SessionService::remove(crow::websocket::connection &conn)
{
    std::unique_lock lock(mtx_);
    auto it = byConnection_.find(&conn);
    if (it == byConnection_.end())
        return;

    const utils::UserId userId = it->second.userId;
    byConnection_.erase(it);

    auto userIt = byUser_.find(userId);
    if (userIt == byUser_.end())
        return;
    userIt->second.erase(&conn);
    if (userIt->second.empty())
        byUser_.erase(userIt);
}
std::optional<utils::UserId> SessionService::userIdOf(crow::websocket::connection &conn) const
{
    std::shared_lock lock(mtx_);
    auto it = byConnection_.find(&conn);
    if (it == byConnection_.end())
        return std::nullopt;
    return it->second.userId;
}

void SessionService::sendToConnection(crow::websocket::connection &conn, const std::string &text) const
{
    conn.send_text(text);
}

void SessionService::sendToUser(const utils::UserId userId, const std::string &text) const
{
    std::vector<crow::websocket::connection *> targets;
    {
        std::shared_lock lock(mtx_);
        auto it = byUser_.find(userId);
        if (it == byUser_.end())
            return;
        targets.assign(it->second.begin(), it->second.end());
    }
    for (auto *conn : targets)
        conn->send_text(text);
    return;
}
void SessionService::sendToUserExcept(const utils::UserId userId, const crow::websocket::connection &except, const std::string &text) const
{
    std::vector<crow::websocket::connection *> targets;
    {
        std::shared_lock lock(mtx_);
        auto it = byUser_.find(userId);
        if (it == byUser_.end())
            return;
        for (auto *conn : it->second)
            if (conn != &except)
                targets.push_back(conn);
    }
    for (auto *conn : targets)
        conn->send_text(text);
    return;
}

void SessionService::sendToUsers(const std::span<utils::UserId> usersId, const std::string &message) const
{
    for (const auto &id : usersId)
        sendToUser(id, message);
}
void SessionService::sendToUsersExcept(const std::span<utils::UserId> usersId, const crow::websocket::connection &except, const std::string &message) const
{
    for (const auto &id : usersId)
        sendToUserExcept(id, except, message);
}

bool SessionService::isOnline(utils::UserId userId) const
{
    std::shared_lock lock(mtx_);
    return byUser_.contains(userId);
}

} // namespace session
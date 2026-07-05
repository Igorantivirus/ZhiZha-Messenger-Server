#include <AuthService/Subservices/AccessTokenStore.hpp>

namespace auth
{

void AccessTokenStore::save(const std::string &accessToken, AccessRecord record)
{
    std::unique_lock lock(mtx_);
    records_.insert_or_assign(accessToken, record);
}

std::optional<AccessRecord> AccessTokenStore::find(const std::string &accessToken) const
{
    std::shared_lock lock(mtx_);
    auto it = records_.find(accessToken);
    if (it == records_.end())
        return std::nullopt;
    return it->second;
}

void AccessTokenStore::remove(const std::string &accessToken)
{
    std::unique_lock lock(mtx_);
    records_.erase(accessToken);
}

void AccessTokenStore::removeAllForUser(utils::UserId userId)
{
    std::unique_lock lock(mtx_);
    std::erase_if(records_, [userId](const auto &pair)
    {
        return pair.second.userId == userId;
    });
}

void AccessTokenStore::removeExpired(std::time_t now)
{
    std::unique_lock lock(mtx_);
    std::erase_if(records_, [now](const auto &pair)
    {
        return pair.second.expiresAt < now;
    });
}

} // namespace auth

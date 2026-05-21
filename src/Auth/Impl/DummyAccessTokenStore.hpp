#pragma once

#include <shared_mutex>
#include <unordered_map>

#include <Auth/Interfaces/IAccessTokenStore.hpp>

class DummyAccessTokenStore : public IAccessTokenStore
{
public:
    void save(const std::string &accessToken, AccessRecord record) override
    {
        std::unique_lock lock(mtx_);
        records_[hashToken(accessToken)] = record;
    }

    std::optional<AccessRecord> find(const std::string &accessToken) const override
    {
        std::shared_lock lock(mtx_);
        auto it = records_.find(hashToken(accessToken));
        if (it == records_.end())
            return std::nullopt;
        return it->second;
    }

    void remove(const std::string &accessToken) override
    {
        std::unique_lock lock(mtx_);
        records_.erase(hashToken(accessToken));
    }

    void removeAllForUser(protocol::UserId userId) override
    {
        std::unique_lock lock(mtx_);
        std::erase_if(records_, [userId](const auto &pair)
        {
            return pair.second.userId == userId;
        });
    }

    unsigned removeExpired(int64_t now) override
    {
        std::unique_lock lock(mtx_);
        int before = records_.size();
        std::erase_if(records_, [now](const auto &pair)
        {
            return pair.second.expiresAt < now;
        });
        return before - records_.size();
    }

private:
    static std::string hashToken(const std::string &token)
    {
        std::hash<std::string> h;
        return std::to_string(h(token));
    }

    mutable std::shared_mutex mtx_;
    std::unordered_map<std::string, AccessRecord> records_;
};
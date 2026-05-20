#pragma once

#include <ranges>
#include <string>
#include <string_view>

#include <Utils/Random.hpp>

#include <Auth/Interfaces/IPasswordHasher.hpp>

class DummyPasswordHasher : public IPasswordHasher
{
public:
    DummyPasswordHasher()
    {
        rnd::StringSettings setts = rnd::StringSettings::allow();
        rand_.setSettings(setts);
        rand_.setDefaultLength(32);
    }
    std::string hash(const std::string &password) override
    {
        std::string salt = rand_.generate();
        std::size_t hashCode = hasher_(password + salt);

        return "dummyalgorithm$v=1$" + salt + '$' + std::to_string(hashCode);
    }

    bool verify(const std::string &password, const std::string &passwordHash) override
    {
        auto parts = passwordHash | std::views::split('$') | std::views::transform([](auto part)
        {
            return std::string_view(part.begin(), part.end());
        }) | std::ranges::to<std::vector>();

        if (parts.size() != 4)
            return false;
        if (parts[0] != "dummyalgorithm")
            return false;
        if (parts[1] != "v=1")
            return false;

        std::string_view salt = parts[2];
        std::string_view storedHash = parts[3];
        std::string computedHash = std::to_string(hasher_(password + std::string(salt)));
        return computedHash == storedHash;
    }

private:
    rnd::RandomString rand_;
    std::hash<std::string> hasher_;
};
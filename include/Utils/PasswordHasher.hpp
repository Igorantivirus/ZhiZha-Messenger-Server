#pragma once

#include <string>

#include "Random.hpp"

namespace utils
{

class PasswordHasher
{
public:
    PasswordHasher();

    std::string hash(const std::string &password);
    bool verify(const std::string &password, const std::string &passwordHash);

private:
    utils::RandomString rand_;
    std::hash<std::string> hasher_;
};

} // namespace utils
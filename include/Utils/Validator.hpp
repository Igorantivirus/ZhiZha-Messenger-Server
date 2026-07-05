#pragma once

#include <string>

#include "StringRule.hpp"

namespace utils
{

class Validator
{
public:
    Validator(StringRule username, StringRule password, StringRule displayName, StringRule roomName);

    bool isValidUsername(const std::string &username) const;
    bool isValidPassword(const std::string &password) const;
    bool isValidDisplayName(const std::string &displayName) const;
    bool isValidRoomName(const std::string &roomName) const;

private:
    StringRule username_;
    StringRule password_;
    StringRule displayName_;
    StringRule roomName_;
};

} // namespace utils
#pragma once

#include <Utils/StringRule.hpp>

namespace app
{
struct ValidationConfig
{
    std::size_t maxMessageSize;
    utils::StringRule username;
    utils::StringRule password;
    utils::StringRule displayName;
    utils::StringRule roomName;
};
} // namespace app
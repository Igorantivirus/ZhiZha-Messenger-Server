#pragma once

#include <Utils/Types.hpp>

#include "TokenPair.hpp"

namespace auth
{

struct AuthSuccess
{
    utils::UserId userId;
    TokenPair tokens;
};

} // namespace auth
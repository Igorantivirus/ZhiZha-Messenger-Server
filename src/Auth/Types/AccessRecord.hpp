#pragma once

#include <ctime>

#include <Protocol/Types.hpp>

struct AccessRecord
{
    protocol::UserId userId;
    std::time_t expiresAt;
};
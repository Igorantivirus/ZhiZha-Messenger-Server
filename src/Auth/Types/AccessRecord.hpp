#pragma once

#include <ctime>

#include <Utils/Types.hpp>

struct AccessRecord
{
    UserId userId;
    std::time_t expiresAt;
};
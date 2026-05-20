#pragma once

#include <cstdint>
#include <ctime>

static std::time_t getCurrentTime()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}
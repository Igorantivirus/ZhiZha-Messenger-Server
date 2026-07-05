#pragma once

#include <ctime>

namespace utils
{
inline std::time_t getCurrentTime()
{
    return std::time(nullptr);
}
}
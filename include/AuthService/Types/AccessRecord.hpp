#pragma once

#include <Utils/Types.hpp>

namespace auth
{
struct AccessRecord
{
    utils::UserId userId;
    std::time_t expiresAt; // Время истечения
};
} // namespace auth
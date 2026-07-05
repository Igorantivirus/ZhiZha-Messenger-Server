#pragma once

#include <Utils/Types.hpp>

namespace auth
{

struct RefreshRecord
{
    utils::UserId userId;  // id пользователя
    std::time_t issuedAt;  // время выда чи
    std::time_t expiresAt; // время истечения срока
};

} // namespace auth
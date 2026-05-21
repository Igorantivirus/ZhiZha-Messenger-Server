#pragma once

#include <Protocol/Types.hpp>
#include <ctime>


struct RefreshRecord
{
    protocol::UserId userId;         // id пользователя
    std::time_t issuedAt;  // время выдачи
    std::time_t expiresAt; // время истечения срока
};
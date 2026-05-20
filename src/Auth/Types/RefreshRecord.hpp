#pragma once

#include <Utils/Types.hpp>
#include <ctime>


struct RefreshRecord
{
    UserId userId;         // id пользователя
    std::time_t issuedAt;  // время выдачи
    std::time_t expiresAt; // время истечения срока
};
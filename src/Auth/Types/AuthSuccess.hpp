#pragma once

#include <ctime>

#include <Utils/Types.hpp>

#include <Auth/Types/TokenPair.hpp>

// Результат успешной аутентификации: пара токенов плюс срок их жизни.
// ttl нужен транспорту, чтобы отдать клиенту accessExpiresIn/refreshExpiresIn.
struct AuthSuccess
{
    UserId userId;
    TokenPair tokens;
    std::time_t accessTtl;  // секунд до истечения access
    std::time_t refreshTtl; // секунд до истечения refresh
};

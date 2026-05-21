#pragma once

#include <ctime>
#include <string>

#include "ErrorCode.hpp"

namespace protocol::api
{

// Тело ошибки для HTTP-ответов (на известном пути type не нужен).
struct ErrorResponse
{
    ErrorCode code;      // машинно-читаемый код
    std::string message; // человеческий текст для UI
};

// discovery-endpoint: клиент перед коннектом узнаёт, куда ходить.
struct InfoResponse
{
    std::string serverName;
    std::string version;
    std::string wsEndpoint;
    std::time_t accessTtl;  // секунды жизни access
    std::time_t refreshTtl; // секунды жизни refresh
};


} // namespace protocol::api

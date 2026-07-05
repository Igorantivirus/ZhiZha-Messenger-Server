#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#include <Protocol/Common/Parsing.hpp>
#include <Protocol/Data/Server.hpp>

namespace protocol::dto
{

// Ответ состояния сервера
struct ServerHealthResponseDto
{
    data::ServerStatus status; // Статус сервера
};
PROTOCOL_JSON_SEREALIZE(ServerHealthResponseDto)

// Ответ информации о сервере
struct ServerInfoResponseDto
{
    std::string serverName;      // Имя сервера
    std::string version;         // Версия сервера
    std::string wsEndpoint;      // Куда запрашивать WebSocket
    std::time_t accessTtl;       // секунды жизни access
    std::time_t refreshTtl;      // секунды жизни refresh
    std::int64_t maxMessageSize; // Максимальная длина сообщения
};
PROTOCOL_JSON_SEREALIZE(ServerInfoResponseDto)

} // namespace protocol::dto

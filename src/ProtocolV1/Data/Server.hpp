#pragma once

#include <cstdint>

#include <ProtocolV1/Common/Parsing.hpp>

namespace protocol::data
{

// Состояние сервера
enum class ServerStatus : std::uint8_t
{
    Ok,      // Сервер работает штатно
    Failed,  // Сервер упал
    Serviced // Плановое обслуживание
};
PROTOCOL_JSON_SEREALIZE(ServerStatus)

} // namespace protocol::data
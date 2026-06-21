#pragma once

#include <cstdint>

#include <ProtocolV1/Common/Parsing.hpp>

namespace protocol
{
using UserId = std::int64_t;
using RoomId = std::int64_t;
using MessageId = std::int64_t;

// Страна
enum class Country : std::uint8_t
{
    None, // не указана
    Ru,   // Россия
    By,   // Беларусь
    Kz,   // Казахстан
    Ua,   // Украина
    Us,   // США
    Gb    // Великобритания
};
PROTOCOL_JSON_SEREALIZE(Country)

} // namespace protocol
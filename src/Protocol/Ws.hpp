#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include <Protocol/Api.hpp>
#include <Utils/Types.hpp>

// По WebSocket заранее неизвестно, какая структура придёт следующей,
// поэтому каждое сообщение несёт дискриминатор type. Формат плоский:
//   { "type": "sendMessage", ...поля... }
// Диспетчеризация: прочитать "type" -> WsMessageType -> десериализовать
// в конкретную структуру.
namespace protocol::ws
{

enum class WsMessageType
{
    unknown,     // нераспознанный тип (fallback при десериализации)
    ping,        // клиент -> сервер: проверка живости
    pong,        // сервер -> клиент: ответ на ping
    error,       // сервер -> клиент: ошибка (может прийти в любой момент)
    sendMessage, // клиент -> сервер: отправить сообщение в чат (заглушка под ChatService)
    newMessage   // сервер -> клиент: новое сообщение в чате (заглушка под ChatService)
};

NLOHMANN_JSON_SERIALIZE_ENUM(WsMessageType, {
    {WsMessageType::unknown, "unknown"},
    {WsMessageType::ping, "ping"},
    {WsMessageType::pong, "pong"},
    {WsMessageType::error, "error"},
    {WsMessageType::sendMessage, "sendMessage"},
    {WsMessageType::newMessage, "newMessage"},
})

// Лёгкий конверт: парсим только type, чтобы понять, что десериализовать дальше.
struct Envelope
{
    WsMessageType type = WsMessageType::unknown;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Envelope, type)

struct Ping
{
    WsMessageType type = WsMessageType::ping;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Ping, type)

struct Pong
{
    WsMessageType type = WsMessageType::pong;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Pong, type)

// Ошибка, отправляемая по WS. Переиспользует общий protocol::api::ErrorCode.
struct ErrorMessage
{
    WsMessageType type = WsMessageType::error;
    protocol::api::ErrorCode code = protocol::api::ErrorCode::unknown;
    std::string message;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ErrorMessage, type, code, message)

// --- Заглушки под будущий ChatService ---

struct SendMessageRequest
{
    WsMessageType type = WsMessageType::sendMessage;
    std::int64_t chatId = 0;
    std::string text;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageRequest, type, chatId, text)

struct NewMessage
{
    WsMessageType type = WsMessageType::newMessage;
    std::int64_t chatId = 0;
    UserId senderId = 0;
    std::string text;
    std::time_t sentAt = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NewMessage, type, chatId, senderId, text, sentAt)

} // namespace protocol::ws

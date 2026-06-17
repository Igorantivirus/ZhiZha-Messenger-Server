#pragma once

#include <ctime>
#include <string>

#include "Rooms.hpp"

#include "ErrorCode.hpp"
#include "Types.hpp"

namespace protocol::ws
{

// ─────────────────────────────────────────────────────────────
// Лёгкий конверт: парсим только type, чтобы понять, что десериализовать дальше.
// ─────────────────────────────────────────────────────────────
enum class WsMessageType
{
    Unknown, // нераспознанный тип (fallback при десериализации)

    Ping,        // клиент -> сервер: проверка живости
    SendMessage, // клиент -> сервер: отправить сообщение в чат
    // Создание и покидание комнаты делаются через HTTP REST, не через WS.

    Pong,        // сервер -> клиент: ответ на ping
    Error,       // сервер -> клиент: ошибка (может прийти в любой момент)
    NewMessage,  // сервер -> клиент: новое сообщение в чате
    MessageAck,  // сервер -> клиент: подтверждение отправителю (локальный id -> глобальный)

    // ── События о комнатах (сервер -> клиент). Команды идут по HTTP,
    //    эти события только информируют онлайн-клиентов о результате. ──
    RoomCreated, // пользователя добавили в новую комнату (несёт Room)
    RoomDeleted, // комната удалена (несёт roomId)
    RoomUpdated, // изменилась информация о комнате (несёт Room)
    UserJoined,  // в комнату добавлен новый участник
    UserLeft,    // участник покинул комнату
    UserUpdated  // пользователь изменил свой профиль (несёт public-инфо)
};
struct Envelope
{
    WsMessageType type = WsMessageType::Unknown;
};
} // namespace protocol::ws

// ─────────────────────────────────────────────────────────────
// Клиент → сервер
// ─────────────────────────────────────────────────────────────
namespace protocol::ws
{
struct Ping
{
    WsMessageType type = WsMessageType::Ping;
};

struct SendMessageRequest
{
    WsMessageType type = WsMessageType::SendMessage;
    RoomId roomId = 0;
    std::string text;
    // Локальный id сообщения, выданный клиентом. Сервер его не хранит —
    // только возвращает в MessageAckEvent, чтобы клиент связал свой
    // временный id с присвоенным глобальным messageId.
    MessageId usersMessageId = 0;
};
} // namespace protocol::ws

// ─────────────────────────────────────────────────────────────
// Сервер → клиент
// ─────────────────────────────────────────────────────────────
namespace protocol::ws
{

struct Pong
{
    WsMessageType type = WsMessageType::Pong;
};

struct ErrorMessage
{
    WsMessageType type = WsMessageType::Error;
    protocol::ErrorCode code = protocol::ErrorCode::Unknown;
    std::string message;
};

// Новое сообщение в комнате. Рассылается ВСЕМ участникам, КРОМE того одного
// соединения, что прислало сообщение — оно вместо этого получает MessageAck.
// Другие устройства отправителя получают NewMessage как обычные участники и
// по senderId == собственный userId понимают, что это их же сообщение,
// отправленное с другого устройства.
struct NewMessageEvent
{
    WsMessageType type = WsMessageType::NewMessage;
    MessageId messageId = 0;
    RoomId roomId = 0;
    UserId senderId = 0;
    std::string text;
    std::time_t createdAt = 0;
};

// Подтверждение отправителю, что сообщение сохранено. Шлётся ТОЛЬКО тому
// соединению, что прислало SendMessageRequest (не всем устройствам юзера).
// Связывает локальный usersMessageId с присвоенным глобальным messageId и
// несёт серверное время создания — отправившее соединение по usersMessageId
// заменяет свой временный id на серверный. NewMessage этому соединению при
// этом НЕ приходит (иначе сообщение задвоилось бы).
struct MessageAckEvent
{
    WsMessageType type = WsMessageType::MessageAck;
    MessageId usersMessageId = 0; // локальный id клиента (из SendMessageRequest)
    MessageId messageId = 0;      // присвоенный сервером глобальный id
    RoomId roomId = 0;
    std::time_t createdAt = 0;
};

// Пользователя добавили в новую комнату. Несёт полную Room, чтобы клиент
// отрисовал её в списке без дополнительного HTTP-запроса.
struct RoomCreatedEvent
{
    WsMessageType type = WsMessageType::RoomCreated;
    rooms::Room room;
};

// Комната удалена. Клиент убирает её из списка.
struct RoomDeletedEvent
{
    WsMessageType type = WsMessageType::RoomDeleted;
    RoomId roomId = 0;
};

// Изменилась информация о комнате (имя/политики). Несёт актуальную Room.
struct RoomUpdatedEvent
{
    WsMessageType type = WsMessageType::RoomUpdated;
    rooms::Room room;
};

// В комнату добавлен новый участник.
struct UserJoinedEvent
{
    WsMessageType type = WsMessageType::UserJoined;
    RoomId roomId = 0;
    UserId userId = 0;
};

// Участник покинул комнату.
struct UserLeftEvent
{
    WsMessageType type = WsMessageType::UserLeft;
    RoomId roomId = 0;
    UserId userId = 0;
};

// Пользователь изменил свой профиль. Несёт актуальную public-инфо, чтобы
// клиенты-получатели обновили кеш без дополнительного HTTP-запроса.
struct UserUpdatedEvent
{
    WsMessageType type = WsMessageType::UserUpdated;
    UserId userId = 0;
    users::UserDisplayInfo display;
};

} // namespace protocol::ws

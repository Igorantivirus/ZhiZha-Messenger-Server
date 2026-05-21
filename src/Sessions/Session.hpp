#pragma once

#include <crow/crow.h>

#include <Protocol/Types.hpp>

// Одно живое WebSocket-подключение аутентифицированного пользователя.
// connection принадлежит Crow; здесь хранится только указатель, валидный
// от onopen до onclose этого соединения.
struct Session
{
    protocol::UserId userId;
    crow::websocket::connection *connection;
};

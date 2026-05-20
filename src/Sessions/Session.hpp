#pragma once

#include <crow/crow.h>

#include <Utils/Types.hpp>

// Одно живое WebSocket-подключение аутентифицированного пользователя.
// connection принадлежит Crow; здесь хранится только указатель, валидный
// от onopen до onclose этого соединения.
struct Session
{
    UserId userId;
    crow::websocket::connection *connection;
};

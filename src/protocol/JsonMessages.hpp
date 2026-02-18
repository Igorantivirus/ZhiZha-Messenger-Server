#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/Types.hpp"

// Клиент -> Сервер: начальная регистрация после подключения по websocket.
struct ClientRegisterRequest
{
    std::string type = "register";      // Тип сообщения: "register".
    std::string publicKey;              // Открытый ключ клиента в виде простого текста для текущего упрощённого потока.
    std::string username;               // Отображаемое имя; может быть пустым.
    std::string password;               // Пароль пользователя
    std::string clientVersion;          // Версия клиентского приложения; может быть пустой.
};

// Клиент -> Сервер: отправка сообщения в комнату.
struct ClientChatMessageRequest
{
    std::string type = "chat-msg";      // Тип сообщения: "chat-msg".
    IDType userId = 0;                  // ID отправителя, полученный после регистрации.
    IDType chatId = 0;                  // ID целевой комнаты.
    std::string message;                // Текст сообщения; пока может быть пустым.
    std::uint64_t clientMessageId = 0;  // Опциональный ID сообщения со стороны клиента; 0 означает, что не указан.
};

// Клиент -> Сервер: создание новой комнаты с выбранными пользователями.
struct ClientCreateRoomRequest
{
    std::string type = "create-room";        // Тип сообщения: "create-room".
    IDType userId = 0;                       // ID пользователя, создающего комнату.
    std::vector<IDType> participantUserIds;  // Пользователи, которых нужно включить в комнату.
    bool isPrivate = true;                   // true = приватная комната, false = публичная комната.
};

// Клиент -> Сервер: выход из существующей комнаты.
struct ClientLeaveRoomRequest
{
    std::string type = "leave-room";     // Тип сообщения: "leave-room".
    IDType userId = 0;                   // Пользователь, желающий покинуть комнату.
    IDType chatId = 0;                   // ID комнаты, которую нужно покинуть.
};

// Сервер -> Клиент: отправляется сразу после открытия websocket до регистрации.
struct ServerHelloPayload
{
    std::string type = "hello";               // Тип сообщения: "hello".
    bool authorized = false;                  // false до успешной регистрации.
    std::uint32_t registrationTimeoutSeconds; // Максимальное время в секундах для завершения регистрации.
    std::string serverName;                   // Человекочитаемое имя сервера.
};

// Сервер -> Клиент: результат регистрации.
struct ServerRegistrationPayload
{
    std::string type = "register-result"; // Message kind: "register-result".
    bool registered = false;             // Статус регистрации: true = успешно, false = ошибка.
    IDType userId = 0;                   // Присвоенный ID пользователя при успешной регистрации.
    std::string serverPublicKey;         // Заглушка открытого ключа сервера.
    std::vector<IDType> usersChats;      // ID комнат, в которых пользователь сейчас участвует.
    std::string serverName;              // Человекочитаемое имя сервера.
    std::string protocolVersion = "1.0"; // Версия протокола, сейчас "1.0".
};

// Сервер -> Клиент: общая ошибка для некорректных запросов.
struct ServerErrorPayload
{
    std::string type = "error";          // Тип сообщения: "error".
    std::string code;                    // Машиночитаемый код ошибки.
    std::string message;                 // Человекочитаемое описание ошибки.
};

// Сервер -> Клиент: широковещательная рассылка сообщения в чате.
struct ServerChatMessagePayload
{
    std::string type = "chat-msg";       // Тип сообщения: "chat-msg".
    IDType userId = 0;                   // ID пользователя-отправителя.
    std::string userName;                // Имя пользователя
    IDType chatId = 0;                   // ID комнаты, в которую отправлено сообщение.
    std::string message;                 // Текст сообщения.
    std::uint64_t serverMessageId = 0;   // Монотонно возрастающий ID сообщения на стороне сервера.
};

// Сервер -> Клиент: ответ на создание комнаты.
struct ServerRoomCreatedPayload
{
    std::string type = "room-created";       // Тип сообщения: "room-created".
    bool created = false;                    // true, если комната создана.
    IDType chatId = 0;                       // ID новой комнаты.
    std::vector<IDType> participantUserIds;  // Фактические ID участников, добавленных в комнату.
};

// Сервер -> Клиент: ответ на выход из комнаты.
struct ServerRoomLeftPayload
{
    std::string type = "room-left";      // Тип сообщения: "room-left".
    bool left = false;                   // true, если пользователь покинул комнату.
    IDType userId = 0;                   // ID пользователя, покинувшего комнату.
    IDType chatId = 0;                   // ID комнаты.
};

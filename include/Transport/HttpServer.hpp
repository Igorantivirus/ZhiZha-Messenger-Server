#pragma once

#include <crow/crow.h>

#include <AuthService/AuthService.hpp>
#include <AuthService/UserQueryService.hpp>
#include <ChatService/ChatQueryService.hpp>
#include <ChatService/ChatService.hpp>
#include <SessionService/SessionService.hpp>

#include <Transport/Controllers/ApiController.hpp>
#include <Transport/Controllers/AuthController.hpp>
#include <Transport/Controllers/MessagesController.hpp>
#include <Transport/Controllers/RoomsController.hpp>
#include <Transport/Controllers/UsersController.hpp>

namespace transport
{

// HTTP-сервер: держит все HTTP-контроллеры. Всё «снаружи» приходит
// исключительно по ссылке — HttpServer никем не владеет, только соединяет
// маршруты Crow с сервисами. Владение живёт в ServerApplication.
class HttpServer
{
public:
    HttpServer(crow::SimpleApp &app,
               session::SessionService &session,
               auth::AuthService &auth,
               auth::UserQueryService &userQuery,
               chat::ChatService &chat,
               chat::ChatQueryService &chatQuery,
               std::time_t accessTtl,
               std::time_t refreshTtl,
               unsigned maxMessageSize);

    void registerRoutes();

private:
    ApiController apiController_;
    AuthController authController_;
    UsersController usersController_;
    RoomsController roomsController_;
    MessagesController messagesController_;
};

} // namespace transport

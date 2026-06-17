#pragma once

#include "ChatService/Interfaces/IRoomRepository.hpp"
#include <crow/crow.h>

#include <Auth/AuthService.hpp>
#include <Auth/Interfaces/IUserRepository.hpp>
#include <ChatService/ChatService.hpp>

#include <Transport/Controllers/ApiController.hpp>
#include <Transport/Controllers/AuthController.hpp>
#include <Transport/Controllers/MessagesController.hpp>
#include <Transport/Controllers/RoomsController.hpp>
#include <Transport/Controllers/UsersController.hpp>

// Корневой HTTP-слой. Владеет всеми контроллерами по значению,
// связывает их с crow::SimpleApp.
class HttpServer
{
public:
    HttpServer(crow::SimpleApp &app,
               AuthService &auth,
               ChatService &chat,
               IUserRepository &userRepo,
               IRoomRepository &roomsRepo,
               std::time_t accessTtl,
               std::time_t refreshTtl,
               std::int64_t maxMessageSize)
        : apiController_(app, accessTtl, refreshTtl, maxMessageSize),
          authController_(app, auth),
          usersController_(app, auth, chat, userRepo),
          roomsController_(app, auth, chat, roomsRepo),
          messagesController_(app, auth, chat)
    {
    }

    void registerRoutes()
    {
        apiController_.registerRoutes();
        authController_.registerRoutes();
        usersController_.registerRoutes();
        roomsController_.registerRoutes();
        messagesController_.registerRoutes();
    }

private:
    ApiController apiController_;
    AuthController authController_;
    UsersController usersController_;
    RoomsController roomsController_;
    MessagesController messagesController_;
};
#include <Transport/HttpServer.hpp>

namespace transport
{

HttpServer::HttpServer(crow::SimpleApp &app,
                       session::SessionService &session,
                       auth::AuthService &auth,
                       auth::UserQueryService &userQuery,
                       chat::ChatService &chat,
                       chat::ChatQueryService &chatQuery,
                       std::time_t accessTtl,
                       std::time_t refreshTtl,
                       unsigned maxMessageSize)
    : apiController_(app, accessTtl, refreshTtl, maxMessageSize),
      authController_(app, auth),
      usersController_(app, auth, userQuery),
      roomsController_(app, session, auth, chat, userQuery, chatQuery),
      messagesController_(app, auth, userQuery, chat, chatQuery)
{
}

void HttpServer::registerRoutes()
{
    apiController_.registerRoutes();
    authController_.registerRoutes();
    usersController_.registerRoutes();
    roomsController_.registerRoutes();
    messagesController_.registerRoutes();
}

} // namespace transport

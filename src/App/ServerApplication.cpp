#include <App/ServerApplication.hpp>

namespace app
{

ServerApplication::ServerApplication(ServerConfig config)
    : config_(std::move(config)),
      // БД открывается с READWRITE|CREATE — если файла нет, создаём.
      // foreign_keys включаем сразу — от него зависят CASCADE в roomMembers/messages.
      db_(config_.databaseFile,
          SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE),
      app_(),
      hasher_(),
      // Validator принимает 4 StringRule из конфига (см. ValidationConfig).
      validator_(config_.validation.username,
                 config_.validation.password,
                 config_.validation.displayName,
                 config_.validation.roomName),

      // Auth-модуль. Порядок важен: userRepo -> stores -> tokens -> сервисы.
      userRepo_(db_),
      accessStore_(),
      refreshStore_(db_),
      tokens_(config_.accessTtl, config_.refreshTtl, accessStore_, refreshStore_),
      authService_(userRepo_, tokens_, hasher_, validator_),
      userQuery_(userRepo_),

      // Chat-модуль. Три репо + два сервиса поверх.
      roomRepo_(db_),
      messageRepo_(db_),
      membersRepo_(db_),
      chatService_(roomRepo_, messageRepo_, membersRepo_),
      chatQuery_(roomRepo_, messageRepo_, membersRepo_),

      sessions_(),

      // Транспорт. HttpServer держит контроллеры, WsServer — WS-роут.
      httpServer_(app_,
                  sessions_,
                  authService_,
                  userQuery_,
                  chatService_,
                  chatQuery_,
                  config_.accessTtl,
                  config_.refreshTtl,
                  static_cast<unsigned>(config_.validation.maxMessageSize)),
      wsServer_(app_,
                chatService_,
                chatQuery_,
                authService_,
                userQuery_,
                sessions_)
{
    // foreign_keys — sqlite-per-connection PRAGMA, включать обязательно ДО
    // работы с CASCADE. Ставим здесь, а не в конструкторе репо: репо не
    // должен решать поведение соединения.
    db_.exec("PRAGMA foreign_keys = ON");
}

void ServerApplication::run()
{
    httpServer_.registerRoutes();
    wsServer_.registerRoutes();

    app_.port(config_.port).multithreaded().run();
}

} // namespace app

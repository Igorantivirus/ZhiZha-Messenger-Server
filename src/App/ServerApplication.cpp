#include <App/ServerApplication.hpp>

#include <filesystem>
#include <iostream>

#include <Tls/TlsProvider.hpp>
#include <Utils/LocalAddress.hpp>

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

    // TLS обязателен: либо пользовательская пара из конфига, либо
    // самоподписанная, сгенерированная (и переиспользуемая) в рабочей
    // директории — там же, где лежит БД.
    const std::filesystem::path workDir =
        std::filesystem::path(config_.databaseFile).parent_path();
    const tls::TlsMaterial tlsMaterial = tls::TlsProvider::prepare(config_.tls, workDir);

    const std::string address = config_.tls.address.empty()
                                    ? utils::detectLocalAddress()
                                    : config_.tls.address;

    // Ссылка-приглашение. Fragment (#...) не передаётся по сети — клиент
    // использует его локально как pinned fingerprint сертификата.
    if (tlsMaterial.fingerprintChanged)
    {
        std::cout << "!!!==================================================!!!\n"
                     "!!!  ССЫЛКА ИЗМЕНИЛАСЬ! Сертификат сервера сменился.  !!!\n"
                     "!!!  Старые ссылки-приглашения больше не действуют.   !!!\n"
                     "!!!==================================================!!!\n";
    }
    std::cout << "Server link:\n"
              << config_.tls.scheme << "://" << address << ':' << config_.port
              << "/#" << tlsMaterial.fingerprint << '\n'
              << (tlsMaterial.selfSigned ? "(self-signed certificate)" : "(user certificate)")
              << std::endl;

    app_.port(config_.port)
        .ssl_file(tlsMaterial.certFile, tlsMaterial.keyFile)
        .multithreaded()
        .run();
}

} // namespace app

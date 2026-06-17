#pragma once

#include <memory>
#include <utility>

#include <SQLiteCpp/SQLiteCpp.h>
#include <crow/crow.h>

#include <App/ServerConfig.hpp>

#include <Auth/AuthService.hpp>
#include <Auth/Impl/DummyAccessTokenStore.hpp>
#include <Auth/Impl/DummyPasswordHasher.hpp>
#include <Auth/Impl/DummyTokenService.hpp>
#include <Auth/Impl/SQLiteRefreshTokenStore.hpp>
#include <Auth/Impl/SQLiteUserRepository.hpp>

#include <ChatService/ChatService.hpp>
#include <ChatService/Impl/SQLiteMessageRepository.hpp>
#include <ChatService/Impl/SQLiteRoomMembersRepository.hpp>
#include <ChatService/Impl/SQLiteRoomRepository.hpp>

#include <Sessions/SessionManager.hpp>
#include <Transport/HttpServer.hpp>
#include <Transport/WsServer.hpp>
#include <Validation/Validator.hpp>

// Композиционный корень. Получает готовый ServerConfig, создаёт конкретные
// реализации, связывает их в граф зависимостей и поднимает оба транспорта
// на одном Crow-приложении.
//
// Порядок объявления полей = порядок инициализации, поэтому зависимости
// объявлены раньше потребителей.
class ServerApplication
{
public:
    explicit ServerApplication(ServerConfig config)
        : // База
          config_(std::move(config)),
          db_(makeDatabase(config_.databaseFile)),
          userRepository_(db_),
          // for Auth
          refreshStore_(db_),
          validator_(config_.validation.username, config_.validation.password, config_.validation.displayName, config_.validation.roomName),
          tokenService_(refreshStore_, accessStore_, static_cast<int>(config_.accessTtl), static_cast<int>(config_.refreshTtl)),
          // for chat
          messageRepository_(db_),
          roomMembersRepository_(db_),
          roomRepository_(db_),
          // доменные шляпы
          sessionManager_(),
          authService_(passwordHasher_, userRepository_, tokenService_, validator_),
          chatService_(roomRepository_, messageRepository_, roomMembersRepository_, sessionManager_, config.validation.maxMessageSize),
          httpServer_(app_, authService_, chatService_, userRepository_, roomRepository_, config_.accessTtl, config_.refreshTtl, config.validation.maxMessageSize),
          // chat пока nullptr: ChatService — рабочий черновик, подключим позже.
          wsServer_(app_, authService_, sessionManager_, chatService_)
    {
        httpServer_.registerRoutes();
        wsServer_.registerRoutes();
    }

    void run()
    {
        app_.port(config_.port).multithreaded().run();
    }

private:
    // Открывает (или создаёт) общую БД сервера и включает контроль внешних ключей.
    static std::shared_ptr<SQLite::Database> makeDatabase(const std::string &fileName)
    {
        auto db = std::make_shared<SQLite::Database>(
            fileName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        db->exec("PRAGMA foreign_keys = ON");
        return db;
    }

    // ── Конфигурация и общие ресурсы ──
    ServerConfig config_;
    std::shared_ptr<SQLite::Database> db_;
    crow::SimpleApp app_;

    // ── Хранилища и реализации Auth ──
    SQLiteUserRepository userRepository_;
    SQLiteRefreshTokenStore refreshStore_;
    DummyAccessTokenStore accessStore_;
    DummyPasswordHasher passwordHasher_;
    validation::Validator validator_;
    DummyTokenService tokenService_;

    // ── Хранилища и реализации Chat ──

    SQLiteMessageRepository messageRepository_;
    SQLiteRoomMembersRepository roomMembersRepository_;
    SQLiteRoomRepository roomRepository_;

    // ── Доменные сервисы ──
    SessionManager sessionManager_;
    AuthService authService_;
    ChatService chatService_;

    // ── Транспорт ──
    HttpServer httpServer_;
    WsServer wsServer_;
};

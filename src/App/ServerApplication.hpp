#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#include <crow/crow.h>

#include <Auth/AuthService.hpp>
#include <Auth/Impl/CredentialsValidator.hpp>
#include <Auth/Impl/DummyAccessTokenStore.hpp>
#include <Auth/Impl/DummyPasswordHasher.hpp>
#include <Auth/Impl/DummyTokenService.hpp>
#include <Auth/Impl/SQLiteRefreshTokenStore.hpp>
#include <Auth/Impl/SQLiteUserRepository.hpp>

#include <Sessions/SessionManager.hpp>
#include <Transport/HttpServer.hpp>
#include <Transport/WsServer.hpp>

// Композиционный корень: создаёт конкретные реализации, связывает их
// в граф зависимостей и поднимает оба транспорта на одном Crow-приложении.
// Порядок объявления полей = порядок инициализации, поэтому зависимости
// объявлены раньше тех, кто их потребляет.
class ServerApplication
{
public:
    ServerApplication()
        : tokenService_(refreshStore_, accessStore_, accessTtl_, refreshTtl_),
          authService_(passwordHasher_, userRepository_, tokenService_, credentialsValidator_),
          httpServer_(app_, authService_, accessTtl_, refreshTtl_),
          // chat пока nullptr: ChatService будет подключён позже.
          wsServer_(app_, authService_, sessionManager_, nullptr)
    {
        httpServer_.registerRoutes();
        wsServer_.registerRoutes();
    }

    void run(std::uint16_t port)
    {
        app_.port(port).multithreaded().run();
    }

private:
    // ── Конфигурация ──
    static constexpr std::time_t accessTtl_ = 900;   // 15 минут
    static constexpr std::time_t refreshTtl_ = 7200; // 2 часа

    // ── Crow-приложение ──
    crow::SimpleApp app_;

    // ── Хранилища и реализации Auth ──
    SQLiteUserRepository userRepository_{"users.db"};
    SQLiteRefreshTokenStore refreshStore_{"tokens.db"};
    DummyAccessTokenStore accessStore_;
    DummyPasswordHasher passwordHasher_;
    CredentialsValidator credentialsValidator_;
    DummyTokenService tokenService_;

    // ── Доменные сервисы ──
    AuthService authService_;
    SessionManager sessionManager_;

    // ── Транспорт ──
    HttpServer httpServer_;
    WsServer wsServer_;
};

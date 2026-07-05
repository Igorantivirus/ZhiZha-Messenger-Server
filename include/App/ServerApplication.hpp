#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <crow/crow.h>

#include <AuthService/AuthService.hpp>
#include <AuthService/Subservices/AccessTokenStore.hpp>
#include <AuthService/Subservices/RefreshTokenStore.hpp>
#include <AuthService/Subservices/TokenService.hpp>
#include <AuthService/Subservices/UserRepository.hpp>
#include <AuthService/UserQueryService.hpp>

#include <ChatService/ChatQueryService.hpp>
#include <ChatService/ChatService.hpp>
#include <ChatService/Subservices/MembersRepository.hpp>
#include <ChatService/Subservices/MessageRepository.hpp>
#include <ChatService/Subservices/RoomRepository.hpp>

#include <SessionService/SessionService.hpp>

#include <Transport/HttpServer.hpp>
#include <Transport/WsServer.hpp>

#include <Utils/PasswordHasher.hpp>
#include <Utils/Validator.hpp>

#include "Configs/ServerConfig.hpp"

namespace app
{

// Composition root. Владеет всеми long-lived объектами: БД, репозиториями,
// сервисами, транспортом. Всё связано ссылками, порядок полей в классе
// = порядок инициализации = порядок жизни (низ модулей → верх модулей).
class ServerApplication
{
public:
    explicit ServerApplication(ServerConfig config);

    void run();

private:
    ServerConfig config_;

    // Ядро: БД. crow::SimpleApp объявлен после — Crow не трогает БД,
    // но по правилу «зависимости живут дольше зависимых».
    SQLite::Database db_;
    crow::SimpleApp app_;

    // Utils (без зависимостей от других модулей проекта).
    utils::PasswordHasher hasher_;
    utils::Validator validator_;

    // Auth: репо -> stores -> tokens -> service + query.
    auth::UserRepository userRepo_;
    auth::AccessTokenStore accessStore_;
    auth::RefreshTokenStore refreshStore_;
    auth::TokenService tokens_;
    auth::AuthService authService_;
    auth::UserQueryService userQuery_;

    // Chat: три репо -> service + query.
    chat::RoomRepository roomRepo_;
    chat::MessageRepository messageRepo_;
    chat::MembersRepository membersRepo_;
    chat::ChatService chatService_;
    chat::ChatQueryService chatQuery_;

    // WS-сессии — общий регистр для WS-сервера и (в будущем) HTTP-контроллеров.
    session::SessionService sessions_;

    // Транспорт.
    transport::HttpServer httpServer_;
    transport::WsServer wsServer_;
};

} // namespace app

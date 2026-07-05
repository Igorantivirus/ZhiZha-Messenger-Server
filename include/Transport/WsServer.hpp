#pragma once

#include "SessionService/SessionService.hpp"
#include <AuthService/AuthService.hpp>
#include <AuthService/UserQueryService.hpp>
#include <ChatService/ChatQueryService.hpp>
#include <ChatService/ChatService.hpp>
#include <SessionService/SessionService.hpp>
#include <crow/crow.h>
#include <nlohmann/json.hpp>

#include <Protocol/Ws/Events.hpp>

namespace transport
{

class WsServer
{
public:
    WsServer(
        crow::SimpleApp &app,
        chat::ChatService &chat,
        chat::ChatQueryService &chatQuery,
        auth::AuthService &auth,
        auth::UserQueryService &userQuery,
        session::SessionService &sessions);

    void registerRoutes();

private:
    crow::SimpleApp &app_;

    chat::ChatService &chat_;
    chat::ChatQueryService &chatQuery_;
    auth::AuthService &auth_;
    auth::UserQueryService &userQuery_;
    session::SessionService &sessions_;

private:
    ///////////////////////////////
    // ЖИЗНЕННЫЙ ЦИКЛ СОЕДИНЕНИЯ //
    ///////////////////////////////

    bool onAccept(const crow::request &req, void **userdata);
    void onOpen(crow::websocket::connection &conn);
    void onMessage(crow::websocket::connection &conn, const std::string &data, bool isBinary);
    void onClose(crow::websocket::connection &conn, const std::string &reason, uint16_t code);

private:
    ////////////////////////////
    // ОБРАБОТКА WS СООБЩЕНИЙ //
    ////////////////////////////

    void handle(crow::websocket::connection &conn, protocol::ws::ErrorEvent ev);
    void handle(crow::websocket::connection &conn, protocol::ws::PingEvent ev);
    void handle(crow::websocket::connection &conn, protocol::ws::PongEvent ev);
    void handle(crow::websocket::connection &conn, protocol::ws::DispatcinghNewMessageEvent ev);
};

} // namespace transport

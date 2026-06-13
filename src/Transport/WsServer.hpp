// Transport/WsServer.hpp
#pragma once

#include "Utils/BindMethod.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <variant>

#include <crow/crow.h>
#include <nlohmann/json.hpp>

#include <Auth/AuthService.hpp>
#include <ChatService/ChatService.hpp>
#include <Protocol/Api.hpp>
#include <Protocol/Parsing.hpp>
#include <Protocol/Types.hpp>
#include <Protocol/Ws.hpp>
#include <Sessions/SessionManager.hpp>
#include <Transport/HttpHelpers.hpp>

// WebSocket-слой. Аутентификация — в handshake: клиент шлёт
// Authorization: Bearer <accessToken>. Невалидный токен => подключение
// отклоняется (HTTP 400 на upgrade). Дальше сервер разбирает конверт
// сообщения по полю type и делегирует доменную работу в IChatService.
//
// chat может быть nullptr, пока ChatService не реализован: тогда чатовые
// сообщения отклоняются с ошибкой, а ping/pong и сессии работают.
class WsServer
{
public:
    WsServer(crow::SimpleApp &app, AuthService &auth, SessionManager &sessions, ChatService &chat)
        : app_(app), auth_(auth), sessions_(sessions), chat_(chat)
    {
    }

    void registerRoutes()
    {
        CROW_WEBSOCKET_ROUTE(app_, "/ws")
            .onaccept(utils::bindMethod(this, &WsServer::onAccept))
            .onopen(utils::bindMethod(this, &WsServer::onOpen))
            .onmessage(utils::bindMethod(this, &WsServer::onMessage))
            .onclose(utils::bindMethod(this, &WsServer::onClose));
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;
    SessionManager &sessions_;
    ChatService &chat_;

private:
    // ─────────────────────────────────────────────────────────────
    // ЖИЗНЕННЫЙ ЦИКЛ СОЕДИНЕНИЯ
    // ─────────────────────────────────────────────────────────────

    // Проверяем токен ещё на этапе handshake. false => Crow ответит 400 и
    // соединение не откроется. При успехе кладём userId в userdata —
    // Crow перенесёт его в connection.userdata(), и оно доживёт до onclose.
    bool onAccept(const crow::request &req, void **userdata)
    {
        std::cout << "Accept\n" << '\n';
        std::optional<protocol::UserId> userId = extractUserId(req);
        if (!userId)
            return false;

        *userdata = new protocol::UserId(*userId);
        return true;
    }

    void onOpen(crow::websocket::connection &conn)
    {
        std::cout << "Open\n" << '\n';
        const auto userId = userIdOf(conn);
        if (!userId)
        {
            // userdata не выставлен — соединение в некорректном состоянии, закрываем.
            conn.close("Missing session");
            return;
        }
        sessions_.add(conn, *userId);
        chat_.onUserConnected(*userId);
    }

    void onMessage(crow::websocket::connection &conn, const std::string &data, bool isBinary)
    {
        if (isBinary)
        {
            sendError(conn, protocol::ErrorCode::InvalidFormat, "Binary frames are not supported");
            return;
        }

        // Единственный путь разбора входящего сообщения — протокольный парсер
        // на основе variant. Никакого ручного разбора JSON здесь нет.
        auto parsed = protocol::ws::parseMessageFromClient(data);
        if (!parsed)
        {
            sendError(conn, mapParsingError(parsed.error()), "Cannot parse message");
            return;
        }

        // Диспетчеризация по фактическому типу варианта.
        std::visit([this, &conn](auto &&msg)
                   { handle(conn, msg); },
                   *parsed);
    }

    void onClose(crow::websocket::connection &conn, const std::string & /*reason*/, uint16_t /*code*/)
    {
        if (auto userId = sessions_.userIdOf(conn); userId)
            chat_.onUserDisconnected(*userId);

        sessions_.remove(conn);

        // userdata выделяли в onAccept — освобождаем здесь.
        delete static_cast<protocol::UserId *>(conn.userdata());
        conn.userdata(nullptr);
    }

private:
    // ─────────────────────────────────────────────────────────────
    // ОБРАБОТЧИКИ ПО ТИПУ СООБЩЕНИЯ (выбираются через std::visit)
    // ─────────────────────────────────────────────────────────────

    void handle(crow::websocket::connection &conn, const protocol::ws::Ping &)
    {
        conn.send_text(nlohmann::json(protocol::ws::Pong{}).dump());
    }

    void handle(crow::websocket::connection &conn, const protocol::ws::SendMessageRequest &request)
    {
        const auto userId = userIdOf(conn);
        if (!userId)
        {
            sendError(conn, protocol::ErrorCode::Unauthorized, "No session for this connection");
            return;
        }

        auto result = chat_.onSendMessage(*userId, conn, request);
        if (!result)
            sendError(conn, HttpHelpers::mapChatErrorWs(result.error()));
    }

    // ErrorMessage входит в MessageFromClient, но клиент не должен слать ошибки
    // серверу — игнорируем такой кадр (можно при желании ответить ошибкой).
    void handle(crow::websocket::connection & /*conn*/, const protocol::ws::ErrorMessage &)
    {
    }

    // ─────────────────────────────────────────────────────────────
    // УТИЛИТЫ
    // ─────────────────────────────────────────────────────────────

    // Извлекает и валидирует access-токен из handshake-заголовка.
    std::optional<protocol::UserId> extractUserId(const crow::request &req) const
    {
        std::string header = req.get_header_value("Authorization");
        if (header.empty() || !header.starts_with("Bearer "))
            return std::nullopt;

        return auth_.validateAccess(header.substr(7));
    }

private:
    // userId соединения. В норме userdata выставлен в onAccept, но проверяем
    // на nullptr явно — без проверки разыменование давало бы UB, если кадр
    // как-то пришёл на соединение без установленного userdata.
    static std::optional<protocol::UserId> userIdOf(crow::websocket::connection &conn)
    {
        auto *ptr = static_cast<protocol::UserId *>(conn.userdata());
        if (!ptr)
            return std::nullopt;
        return *ptr;
    }

    static void sendError(crow::websocket::connection &conn, protocol::ErrorCode code, std::string message)
    {
        sendError(conn, protocol::ws::ErrorMessage{.code = code, .message = std::move(message)});
    }

    static void sendError(crow::websocket::connection &conn, const protocol::ws::ErrorMessage &err)
    {
        conn.send_text(nlohmann::json(err).dump());
    }

    // Ошибка протокольного парсера -> единый код ошибки для клиента.
    static protocol::ErrorCode mapParsingError(protocol::ws::WsParsingError e)
    {
        using protocol::ErrorCode;
        using protocol::ws::WsParsingError;
        switch (e)
        {
        case WsParsingError::EmptyString:
        case WsParsingError::InvalidJson:
            return ErrorCode::InvalidFormat;
        case WsParsingError::NotContainType:
        case WsParsingError::UnparsableType:
        case WsParsingError::InvalidTypeValue:
            return ErrorCode::UnknownMessageType;
        case WsParsingError::Unknown:
            break;
        }
        return ErrorCode::InvalidFormat;
    }
};

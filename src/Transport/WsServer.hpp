// Transport/WsServer.hpp
#pragma once

#include <cstdint>
#include <string>

#include <crow/crow.h>
#include <nlohmann/json.hpp>

#include <Auth/AuthService.hpp>
#include <ChatService/ChatService.hpp>
#include <Protocol/Api.hpp>
#include <Protocol/Parsing.hpp>
#include <Protocol/Types.hpp>
#include <Protocol/Ws.hpp>
#include <Sessions/SessionManager.hpp>

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
            .onaccept([this](const crow::request &req, void **userdata)
        {
            return onAccept(req, userdata);
        }).onopen([this](crow::websocket::connection &conn)
        {
            onOpen(conn);
        }).onmessage([this](crow::websocket::connection &conn, const std::string &data, bool isBinary)
        {
            onMessage(conn, data, isBinary);
        }).onclose([this](crow::websocket::connection &conn, const std::string &reason, uint16_t code)
        {
            onClose(conn, reason, code);
        });
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
        std::optional<protocol::UserId> userId = extractUserId(req);
        if (!userId)
            return false;

        *userdata = new protocol::UserId(*userId);
        return true;
    }

    void onOpen(crow::websocket::connection &conn)
    {
        const protocol::UserId userId = userIdOf(conn);
        sessions_.add(conn, userId);
        chat_.onUserConnected(userId);
    }

    void onMessage(crow::websocket::connection &conn, const std::string &data, bool isBinary)
    {
        if (isBinary)
        {
            sendError(conn, protocol::ErrorCode::InvalidFormat, "Binary frames are not supported");
            return;
        }

        nlohmann::json json;
        try
        {
            json = nlohmann::json::parse(data);
        }
        catch (const std::exception &)
        {
            sendError(conn, protocol::ErrorCode::InvalidFormat, "Message is not valid JSON");
            return;
        }

        // Сначала читаем только дискриминатор, потом — конкретную структуру.
        const auto type = json.value("type", protocol::ws::WsMessageType::Unknown);
        dispatch(conn, type, json);
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
    // ДИСПЕТЧЕРИЗАЦИЯ
    // ─────────────────────────────────────────────────────────────

    void dispatch(crow::websocket::connection &conn, protocol::ws::WsMessageType type, const nlohmann::json &json)
    {
        using protocol::ws::WsMessageType;
        switch (type)
        {
        case WsMessageType::Ping:
            conn.send_text(nlohmann::json(protocol::ws::Pong{}).dump());
            return;

        case WsMessageType::SendMessage:
            handleSendMessage(conn, json);
            return;

        default:
            sendError(conn, protocol::ErrorCode::UnknownMessageType, "Unsupported message type");
            return;
        }
    }

    void handleSendMessage(crow::websocket::connection &conn, const nlohmann::json &json)
    {
        protocol::ws::SendMessageRequest request;
        try
        {
            json.get_to(request);
        }
        catch (const std::exception &)
        {
            sendError(conn, protocol::ErrorCode::InvalidFormat, "Malformed sendMessage payload");
            return;
        }

        void(chat_.onSendMessage(userIdOf(conn), request));
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
    // userId соединения, гарантированно установленный в onAccept.
    static protocol::UserId userIdOf(crow::websocket::connection &conn)
    {
        return *static_cast<protocol::UserId *>(conn.userdata());
    }

    static void sendError(crow::websocket::connection &conn, protocol::ErrorCode code, std::string message)
    {
        protocol::ws::ErrorMessage err{.code = code, .message = std::move(message)};
        conn.send_text(nlohmann::json(err).dump());
    }
};

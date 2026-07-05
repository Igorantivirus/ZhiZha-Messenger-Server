#include "Protocol/Ws/EventParsing.hpp"
#include "Protocol/Ws/Events.hpp"
#include <Transport/WsServer.hpp>

#include <Transport/Helpers.hpp>
#include <Utils/BindMethod.hpp>

namespace transport
{
namespace
{
std::optional<protocol::UserId> userIdOfConnection(crow::websocket::connection &conn)
{
    auto *ptr = static_cast<protocol::UserId *>(conn.userdata());
    if (!ptr)
        return std::nullopt;
    return *ptr;
}

void sendError(crow::websocket::connection &conn, protocol::ErrorCode code, std::string message)
{
    protocol::ws::ErrorEvent err{.error = code, .msg = std::move(message)};
    conn.send_text(nlohmann::json(err).dump());
}

void sendWsEvent(crow::websocket::connection &conn, std::string msg)
{
    conn.send_text(std::move(msg));
}
} // namespace

WsServer::WsServer(crow::SimpleApp &app,
                   chat::ChatService &chat,
                   chat::ChatQueryService &chatQuery,
                   auth::AuthService &auth,
                   auth::UserQueryService &userQuery,
                   session::SessionService &sessions)
    : app_(app),
      chat_(chat),
      chatQuery_(chatQuery),
      auth_(auth),
      userQuery_(userQuery),
      sessions_(sessions)
{
}

void WsServer::registerRoutes()
{

    CROW_WEBSOCKET_ROUTE(app_, protocol::ws::wsEndpoint)
        .onaccept(utils::bindMethod(this, &WsServer::onAccept))
        .onopen(utils::bindMethod(this, &WsServer::onOpen))
        .onmessage(utils::bindMethod(this, &WsServer::onMessage))
        .onclose(utils::bindMethod(this, &WsServer::onClose));
}

///////////////////////////////
// ЖИЗНЕННЫЙ ЦИКЛ СОЕДИНЕНИЯ //
///////////////////////////////

bool WsServer::onAccept(const crow::request &req, void **userdata)
{
    auto userId = Helpers::requireAuth(req, auth_);
    if (!userId)
        return false;
    *userdata = new protocol::UserId(*userId);
    return true;
}

void WsServer::onOpen(crow::websocket::connection &conn)
{
    const auto userId = userIdOfConnection(conn);
    if (!userId)
    {
        // userdata не выставлен — соединение в некорректном состоянии, закрываем.
        conn.close("Missing session");
        return;
    }
    sessions_.add(conn, *userId);
}

void WsServer::onMessage(crow::websocket::connection &conn, const std::string &data, bool isBinary)
{
    if (isBinary)
        return sendError(conn, protocol::ErrorCode::InvalidFormat, "Binary frames are not supported");

    auto parsed = protocol::ws::WsEventParser::parseMessageFromClient(data);
    if (!parsed)
        return sendError(conn, protocol::ws::WsEventParser::mapParsingError(parsed.error()), "Cannot parse message");

    auto visiter = [this, &conn](auto &&msg)
    {
        handle(conn, msg);
    };
    std::visit(std::move(visiter), *parsed);
}

void WsServer::onClose(crow::websocket::connection &conn, const std::string &reason, uint16_t code)
{
    if (auto userId = sessions_.userIdOf(conn); userId)
        void(); // chat_.onUserDisconnected(*userId);

    sessions_.remove(conn);

    // userdata выделяли в onAccept — освобождаем здесь.
    delete static_cast<protocol::UserId *>(conn.userdata());
    conn.userdata(nullptr);
}

////////////////////////////
// ОБРАБОТКА WS СООБЩЕНИЙ //
////////////////////////////

void WsServer::handle(crow::websocket::connection &conn, protocol::ws::ErrorEvent ev)
{
    // TODO: Logging
}
void WsServer::handle(crow::websocket::connection &conn, protocol::ws::PingEvent ev)
{
    sendWsEvent(conn, Helpers::dtoToString(protocol::ws::PongEvent{}));
}
void WsServer::handle(crow::websocket::connection &conn, protocol::ws::PongEvent ev)
{
    // TODO: check
}
void WsServer::handle(crow::websocket::connection &conn, protocol::ws::DispatcinghNewMessageEvent ev)
{
    auto id = userIdOfConnection(conn);
    if (!id)
        conn.close("Missing session");
    auto res = chat_.sendMessage(*id, ev.roomId, ev.text);
    if (!res)
        return sendError(conn, Helpers::mapErrorType(res.error()), "Error of send message");

    protocol::ws::MessageAckEvent ackEvent;
    ackEvent.globalMessageId = res->messageId;
    ackEvent.localMessageId = ev.localMessageId;
    ackEvent.createAt = res->createdAt;
    ackEvent.roomId = ev.roomId;
    sendWsEvent(conn, Helpers::dtoToString(std::move(ackEvent)));

    protocol::ws::SendingNewMessageEvent msgEvent;
    msgEvent.messageId = res->messageId;
    msgEvent.createdAt = res->createdAt;
    msgEvent.roomId = ev.roomId;
    msgEvent.senderId = *id;
    msgEvent.text = std::move(ev.text);

    sessions_.sendToUsersExcept(res->notifiables, conn, Helpers::dtoToString(std::move(msgEvent)));
}

} // namespace transport

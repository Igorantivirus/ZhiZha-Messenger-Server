#pragma once

#include <crow/crow.h>

#include <ranges>

#include <Auth/AuthService.hpp>
#include <ChatService/ChatService.hpp>
#include <Protocol/Messages.hpp>
#include <Transport/HttpHelpers.hpp>

class MessagesController
{
public:
    MessagesController(crow::SimpleApp &app, AuthService &auth, ChatService &chat)
        : app_(app), auth_(auth), chat_(chat)
    {
    }

    void registerRoutes()
    {
        // История / синхронизация
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/messages").methods("GET"_method)([this](const crow::request &req, std::uint64_t roomId)
        {
            return handleGetMessages(req, roomId);
        });

        // Отметить как прочитанное
        // CROW_ROUTE(app_, "/api/v1/rooms/<uint>/read").methods("POST"_method)([this](const crow::request &req, std::uint64_t roomId)
        // {
        //     return handleMarkRead(req, roomId);
        // });
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;
    ChatService &chat_;

    crow::response handleGetMessages(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        // Парсим query: beforeId, afterId, limit
        const auto &qs = req.url_params;
        std::optional<protocol::MessageId> beforeId;
        std::optional<protocol::MessageId> afterId;
        unsigned limit = 50;

        try
        {
            if (const char *s = qs.get("beforeId"))
                beforeId = std::stoull(s);
            if (const char *s = qs.get("afterId"))
                afterId = std::stoull(s);
            if (const char *s = qs.get("limit"))
                limit = std::min<unsigned>(std::stoul(s), 100u);
        }
        catch (...)
        {
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::InvalidFormat, "Invalid query parameters");
        }

        if (beforeId && afterId)
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::InvalidFormat, "Cannot use beforeId and afterId together");

        auto messages = chat_.getMessages(*userId, roomId, beforeId, afterId, limit);
        if (!messages.has_value())
            return HttpHelpers::mapChatError(messages.error());

        protocol::messages::MessagesResponse resp;
        resp.roomId = roomId;
        resp.hasMore = messages.value().hasMore;
        resp.messages = messages.value().messages | std::views::transform([](const Message &msg) -> protocol::messages::Message
        {
            protocol::messages::Message protoMsg;
            protoMsg.id = msg.id;
            protoMsg.text = msg.text;
            protoMsg.createdAt = msg.createdAt;
            protoMsg.fromUserId = msg.fromUserId;
            return protoMsg;
        }) | std::ranges::to<std::vector<protocol::messages::Message>>();
        return HttpHelpers::jsonResponse(200, resp);
    }

    // crow::response handleMarkRead(const crow::request &req, std::uint64_t roomId)
    // {
    //     auto userId = HttpHelpers::requireAuth(req, auth_);
    //     if (!userId)
    //         return HttpHelpers::unauthorizedResponse();

    //     protocol::messages::MarkReadRequest dto;
    //     if (!HttpHelpers::parseBody(req, dto))
    //         return HttpHelpers::invalidFormatResponse();

    //     // TODO: chat_.markRead(*userId, roomId, dto.lastReadMessageId)
    //     //   ─ не участник → 404 или 204 (на вкус; я бы 204 — идемпотентно)
    //     //   ─ ок         → 204 + broadcast read receipt через WS
    //     return crow::response(204);
    // }
};
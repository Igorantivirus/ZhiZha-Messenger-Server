#pragma once

#include "Utils/BindMethod.hpp"
#include "Utils/QueryParamsHelper.hpp"
#include <crow/crow.h>

#include <ranges>
#include <string_view>

#include <Auth/AuthService.hpp>
#include <ChatService/ChatService.hpp>
#include <Protocol/Messages.hpp>
#include <Protocol/Users.hpp>
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
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/messages").methods("GET"_method)(utils::bindMethod(this, &MessagesController::handleGetMessages));

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

        std::optional<protocol::MessageId> beforeId = utils::parseQuery<protocol::MessageId>(qs.get("beforeId"));
        std::optional<protocol::MessageId> afterId = utils::parseQuery<protocol::MessageId>(qs.get("afterId"));
        std::optional<unsigned> limit = utils::parseQuery<unsigned>(qs.get("limit"));

        // Нужен ли клиенту словарь авторов этой страницы. По умолчанию — нет.
        const bool withSenders = qs.get("withSenders") != nullptr && std::string_view(qs.get("withSenders")) == "true";

        if (!limit)
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::InvalidFormat, "Invalid query parameters");
        if (beforeId && afterId)
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::InvalidFormat, "Cannot use beforeId and afterId together");

        auto messages = chat_.getMessages(*userId, roomId, beforeId, afterId, *limit);
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

        // По запросу добираем public-инфо авторов страницы (дедуп по fromUserId),
        // чтобы клиент не делал отдельный запрос участников. Имена живут в Auth.
        if (withSenders)
        {
            for (const auto &msg : resp.messages)
            {
                if (resp.senders.contains(msg.fromUserId))
                    continue;
                auto user = auth_.getUserRepository().findUserById(msg.fromUserId);
                if (!user)
                    continue;
                resp.senders[msg.fromUserId] = protocol::users::UserDisplayInfo{
                    .username = std::move(user->username),
                    .displayname = std::move(user->displayeName),
                    .birthDate = user->birthDate,
                    .country = user->country,
                    .registerTime = user->registerTime};
            }
        }

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
#include <Transport/Controllers/MessagesController.hpp>

#include <unordered_set>
#include <vector>

#include <ChatService/ChatService.hpp>
#include <ChatService/Types/Message.hpp>
#include <Protocol/Data/Messages.hpp>
#include <Protocol/Data/Users.hpp>
#include <Transport/Helpers.hpp>

namespace transport
{

namespace
{

// chat::Message и protocol::data::Message имеют одинаковый набор полей,
// но это разные типы — нужна явная конвертация на границе сервис->транспорт.
protocol::data::Message toDto(const chat::Message &m)
{
    return protocol::data::Message{
        .id = m.id,
        .roomId = m.roomId,
        .fromUserId = m.fromUserId,
        .text = m.text,
        .createdAt = m.createdAt};
}

} // namespace

MessagesController::MessagesController(crow::SimpleApp &app,
                                       auth::AuthService &auth,
                                       auth::UserQueryService &usersQuery,
                                       chat::ChatService &chat,
                                       chat::ChatQueryService &chatQuery)
    : app_(app),
      auth_(auth),
      usersQuery_(usersQuery),
      chat_(chat),
      chatQuery_(chatQuery)
{
}

void MessagesController::registerRoutes()
{
    Helpers::bindWithAuth<GetMessages>(app_, this, auth_, &MessagesController::handleGetMessages);
}

Helpers::HttpResponse<MessagesController::GetMessages, chat::ChatError>
MessagesController::handleGetMessages(Helpers::HttpRequest<GetMessages> req)
{
    auto page = chatQuery_.getMessages(*req.userId, req.path.roomId, req.query.afterId, req.query.beforeId, req.query.limit);
    if (!page)
        return std::unexpected(page.error());

    GetMessages::Response resp;
    resp.roomId = req.path.roomId;
    resp.hasMore = page->hasMore;

    resp.messages.reserve(page->elems.size());
    for (const auto &m : page->elems)
        resp.messages.push_back(toDto(m));

    if (req.query.withSenders)
    {
        std::unordered_set<utils::UserId> uniqueIds;
        uniqueIds.reserve(page->elems.size());
        for (const auto &m : page->elems)
            uniqueIds.insert(m.fromUserId);

        const std::vector<utils::UserId> ids(uniqueIds.begin(), uniqueIds.end());
        
        

        const auto users = usersQuery_.getDisplayInfos(ids);

        resp.senders.reserve(users.size());
        for (const auto &u : users)
            resp.senders.emplace(u.id, protocol::data::UserDisplayInfo{.displayName = u.displayName});
    }

    return resp;
}

} // namespace transport
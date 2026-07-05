#pragma once

#include <crow/crow.h>

#include <AuthService/AuthService.hpp>
#include <AuthService/UserQueryService.hpp>
#include <ChatService/ChatQueryService.hpp>
#include <ChatService/ChatService.hpp>
#include <ChatService/Types/ChatError.hpp>
#include <Protocol/Operations/Messages.hpp>
#include <Transport/Helpers.hpp>

namespace transport
{

class MessagesController
{
public:
    MessagesController(crow::SimpleApp &app,
                       auth::AuthService &auth,
                       auth::UserQueryService &usersQUery,
                       chat::ChatService &chat,
                       chat::ChatQueryService &chatQuery);

    void registerRoutes();

private:
    crow::SimpleApp &app_;
    auth::AuthService &auth_;
    auth::UserQueryService &usersQuery_;
    chat::ChatService &chat_;
    chat::ChatQueryService &chatQuery_;

private:
    using GetMessages = protocol::messages::GetMessagesOperation;

private:
    Helpers::HttpResponse<GetMessages, chat::ChatError> handleGetMessages(Helpers::HttpRequest<GetMessages> req);
};

} // namespace transport
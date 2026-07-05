#pragma once

#include "AuthService/UserQueryService.hpp"
#include <crow/crow.h>

#include <AuthService/AuthService.hpp>
#include <Protocol/Operations/Users.hpp>
#include <Transport/Helpers.hpp>

namespace transport
{

class UsersController
{
public:
    UsersController(crow::SimpleApp &app, auth::AuthService &auth, auth::UserQueryService& userQuery);

    void registerRoutes();

private:
    crow::SimpleApp &app_;
    auth::AuthService &auth_;
    auth::UserQueryService& userQuery_;

private:
    using LoopUsers = protocol::users::LoopUsersOperation;
    using ChangeMe = protocol::users::ChangeMeOperation;
    using GetMe = protocol::users::GetMeOperation;
    using GetUser = protocol::users::GetUserOperation;

private:
    Helpers::HttpResponse<LoopUsers, auth::AuthError> handleLoopUsers(Helpers::HttpRequest<LoopUsers> req);
    Helpers::HttpResponse<ChangeMe, auth::AuthError> handleChangeMe(Helpers::HttpRequest<ChangeMe> req);
    Helpers::HttpResponse<GetMe, auth::AuthError> handleGetMe(Helpers::HttpRequest<GetMe> req);
    Helpers::HttpResponse<GetUser, auth::AuthError> handleGetUser(Helpers::HttpRequest<GetUser> req);
};

} // namespace transport
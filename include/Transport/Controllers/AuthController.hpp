#pragma once

#include <crow/crow.h>

#include <AuthService/AuthService.hpp>
#include <Protocol/Operations/Auth.hpp>
#include <Transport/Helpers.hpp>

namespace transport
{

class AuthController
{

public:
    AuthController(crow::SimpleApp &app, auth::AuthService &auth);

    void registerRoutes();

private:
    crow::SimpleApp &app_;
    auth::AuthService &auth_;

private:
    using Register = protocol::auth::RegisterOperation;
    using Login = protocol::auth::LoginOperation;
    using Refresh = protocol::auth::RefreshOperation;
    using Logout = protocol::auth::LogoutOperation;
    using LogoutAll = protocol::auth::LogoutAllOperation;

private:
    Helpers::HttpResponse<Register, auth::AuthError> handleRegister(Helpers::HttpRequest<Register> req);
    Helpers::HttpResponse<Login, auth::AuthError> handleLogin(Helpers::HttpRequest<Login> req);
    Helpers::HttpResponse<Refresh, auth::AuthError> handleRefresh(Helpers::HttpRequest<Refresh> req);
    Helpers::HttpResponse<Logout, auth::AuthError> handleLogout(Helpers::HttpRequest<Logout> req);
    Helpers::HttpResponse<LogoutAll, auth::AuthError> handleLogoutAll(Helpers::HttpRequest<LogoutAll> req);
};

} // namespace transport
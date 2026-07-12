#include "AuthService/Types/Validations.hpp"
#include "Transport/Helpers.hpp"
#include <Transport/Controllers/AuthController.hpp>

namespace transport
{

AuthController::AuthController(crow::SimpleApp &app, auth::AuthService &auth)
    : app_(app), auth_(auth)
{
}

void AuthController::registerRoutes()
{
    Helpers::bindWithoutAuth<Register>(app_, this, &AuthController::handleRegister);
    Helpers::bindWithoutAuth<Login>(app_, this, &AuthController::handleLogin);
    Helpers::bindWithoutAuth<Refresh>(app_, this, &AuthController::handleRefresh);
    Helpers::bindWithAuth<Logout>(app_, this, auth_, &AuthController::handleLogout);
    Helpers::bindWithAuth<LogoutAll>(app_, this, auth_, &AuthController::handleLogoutAll);
}

Helpers::HttpResponse<AuthController::Register, auth::AuthError> AuthController::handleRegister(Helpers::HttpRequest<Register> req)
{
    auth::UserRegistrate reg;
    reg.username = std::move(req.body.username);
    reg.displayName = std::move(req.body.displayName);
    reg.country = std::move(req.body.country);
    reg.password = std::move(req.body.password);
    reg.birthDate = std::move(req.body.birthDate);

    auto res = auth_.registrate(std::move(reg));
    if (!res)
        return std::unexpected(res.error());

    AuthController::Register::Response resp;
    resp.accessToken = res.value().tokens.access;
    resp.refreshToken = res.value().tokens.refresh;
    resp.userId = res.value().userId;

    return resp;
}
Helpers::HttpResponse<AuthController::Login, auth::AuthError> AuthController::handleLogin(Helpers::HttpRequest<Login> req)
{
    auth::Validations valid;
    valid.password = std::move(req.body.password);
    valid.username = std::move(req.body.username);

    auto res = auth_.login(std::move(valid));
    if (!res)
        return std::unexpected(res.error());

    AuthController::Register::Response resp;
    resp.accessToken = res.value().tokens.access;
    resp.refreshToken = res.value().tokens.refresh;
    resp.userId = res.value().userId;

    return resp;
}
Helpers::HttpResponse<AuthController::Refresh, auth::AuthError> AuthController::handleRefresh(Helpers::HttpRequest<Refresh> req)
{
    auto res = auth_.refresh(req.body.refreshToken);

    if (!res)
        return std::unexpected(res.error());

    AuthController::Register::Response resp;
    resp.accessToken = res.value().tokens.access;
    resp.refreshToken = res.value().tokens.refresh;
    resp.userId = res.value().userId;

    return resp;
}
Helpers::HttpResponse<AuthController::Logout, auth::AuthError> AuthController::handleLogout(Helpers::HttpRequest<Logout> req)
{
    auth_.logout(req.body.refreshToken);
    return {};
}
Helpers::HttpResponse<AuthController::LogoutAll, auth::AuthError> AuthController::handleLogoutAll(Helpers::HttpRequest<LogoutAll> req)
{
    auth_.logoutAll(*req.userId);
    return {};
}

} // namespace transport
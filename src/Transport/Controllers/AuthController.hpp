#pragma once

#include "Auth/Types/AuthError.hpp"
#include "ProtocolV1/Dto/Auth.hpp"
#include <crow/crow.h>

#include <Auth/AuthService.hpp>
#include <Transport/HttpHelpers.hpp>
#include <Utils/BindMethod.hpp>

#include <ProtocolV1/Operations/Auth.hpp>

class AuthController
{
private:
    using Register = protocol::auth::RegisterOperation;
    using Login = protocol::auth::LoginOperation;
    using Refresh = protocol::auth::RegisterOperation;
    using Logout = protocol::auth::LogoutOperation;
    using LogoutAll = protocol::auth::LogoutAllOperation;

public:
    AuthController(crow::SimpleApp &app, AuthService &auth)
        : app_(app), auth_(auth)
    {
    }

    void registerRoutes()
    {
        HttpHelpers::bindWithoutAuth<Register>(app_, this, &AuthController::handleRegister);
        HttpHelpers::bindWithoutAuth<Login>(app_, this, &AuthController::handleRegister);
        HttpHelpers::bindWithoutAuth<Refresh>(app_, this, &AuthController::handleRegister);
        HttpHelpers::bindWithoutAuth<Logout>(app_, this, &AuthController::handleRegister);
        HttpHelpers::bindWithoutAuth<LogoutAll>(app_, this, &AuthController::handleRegister);

        // CROW_ROUTE(app_, "/api/v1/auth/register").methods("POST"_method)(utils::bindMethod(this, &AuthController::handleRegister));
        // CROW_ROUTE(app_, "/api/v1/auth/login").methods("POST"_method)(utils::bindMethod(this, &AuthController::handleLogin));
        // CROW_ROUTE(app_, "/api/v1/auth/refresh").methods("POST"_method)(utils::bindMethod(this, &AuthController::handleRefresh));
        // CROW_ROUTE(app_, "/api/v1/auth/logout").methods("POST"_method)(utils::bindMethod(this, &AuthController::handleLogout));
        // CROW_ROUTE(app_, "/api/v1/auth/logout-all").methods("POST"_method)(utils::bindMethod(this, &AuthController::handleLogoutAll));
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;

private:
    // ─────────────────────────────────────────────────────────────
    // Хэндлеры
    // ─────────────────────────────────────────────────────────────

    HttpHelpers::HttpResponse<Register, AuthError> handleRegister(HttpHelpers::HttpRequest<Register> req)
    {
        Register::Request &body = req.body;
        auto result = auth_.registerUser(body.username, body.password, body.displayName, body.birthDate, body.country);
        if (!result)
            return std::unexpected(result.error());
        protocol::dto::AuthSuccessResponseDto res;
        res.accessToken = result.value().tokens.access;
        res.refreshToken = result.value().tokens.refresh;
        res.userId = result.value().userId;
        return res;
    }

    // crow::response handleRegister(const crow::request &req)
    // {
    //     protocol::auth::RegisterRequest dto;
    //     if (!HttpHelpers::parseBody(req, dto))
    //         return HttpHelpers::invalidFormatResponse();

    //     auto result = auth_.registerUser(dto.username, dto.password, dto.displayName, dto.birthDate, dto.country);
    //     if (!result.has_value())
    //         return HttpHelpers::mapError(result.error());

    //     return successResponse(201, result.value());
    // }

    // crow::response handleLogin(const crow::request &req)
    // {
    //     protocol::auth::LoginRequest dto;
    //     if (!HttpHelpers::parseBody(req, dto))
    //         return HttpHelpers::invalidFormatResponse();

    //     auto result = auth_.login(dto.username, dto.password);
    //     if (!result.has_value())
    //         return HttpHelpers::mapError(result.error());

    //     return successResponse(200, result.value());
    // }

    // crow::response handleRefresh(const crow::request &req)
    // {
    //     // refresh-токен ВСЕГДА в теле, не в заголовке
    //     protocol::auth::RefreshRequest dto;
    //     if (!HttpHelpers::parseBody(req, dto))
    //         return HttpHelpers::invalidFormatResponse();

    //     auto result = auth_.refresh(dto.refreshToken);
    //     if (!result.has_value())
    //         return HttpHelpers::errorResponse(401,
    //                                           protocol::ErrorCode::InvalidRefreshToken,
    //                                           "Refresh token is invalid or expired");

    //     return successResponse(200, result.value());
    // }

    // crow::response handleLogout(const crow::request &req)
    // {
    //     protocol::auth::LogoutRequest dto;
    //     if (!HttpHelpers::parseBody(req, dto))
    //         return HttpHelpers::invalidFormatResponse();

    //     // logout идемпотентен — даже если такого refresh уже нет, ответ тот же
    //     auth_.logout(dto.refreshToken);
    //     return crow::response(204);
    // }

    // crow::response handleLogoutAll(const crow::request &req)
    // {
    //     // Этот endpoint требует ACCESS-токен (а не refresh) — из заголовка
    //     auto userId = HttpHelpers::requireAuth(req, auth_);
    //     if (!userId)
    //         return HttpHelpers::unauthorizedResponse();

    //     auth_.logoutAll(*userId);
    //     return crow::response(204);
    // }

private:
    // ─────────────────────────────────────────────────────────────
    // Внутренние утилиты
    // ─────────────────────────────────────────────────────────────
    crow::response successResponse(int code, const AuthSuccess &s) const
    {
        protocol::dto::AuthSuccessResponseDto dto{
            .userId = s.userId,
            .accessToken = s.tokens.access,
            .refreshToken = s.tokens.refresh};
        return HttpHelpers::jsonResponse(code, dto);
    }
};
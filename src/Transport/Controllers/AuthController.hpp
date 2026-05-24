#pragma once

#include <crow/crow.h>

#include <Auth/AuthService.hpp>
#include <Protocol/Auth.hpp>
#include <Transport/HttpHelpers.hpp>

class AuthController
{
public:
    AuthController(crow::SimpleApp &app, AuthService &auth)
        : app_(app), auth_(auth)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/auth/register").methods("POST"_method)([this](const crow::request &req)
        {
            return handleRegister(req);
        });

        CROW_ROUTE(app_, "/api/v1/auth/login").methods("POST"_method)([this](const crow::request &req)
        {
            return handleLogin(req);
        });

        CROW_ROUTE(app_, "/api/v1/auth/refresh").methods("POST"_method)([this](const crow::request &req)
        {
            return handleRefresh(req);
        });

        CROW_ROUTE(app_, "/api/v1/auth/logout").methods("POST"_method)([this](const crow::request &req)
        {
            return handleLogout(req);
        });

        CROW_ROUTE(app_, "/api/v1/auth/logout-all").methods("POST"_method)([this](const crow::request &req)
        {
            return handleLogoutAll(req);
        });
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;

private:
    // ─────────────────────────────────────────────────────────────
    // Хэндлеры
    // ─────────────────────────────────────────────────────────────

    crow::response handleRegister(const crow::request &req)
    {
        protocol::auth::RegisterRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        auto result = auth_.registerUser(dto.username, dto.password, dto.displayName);
        if (!result.has_value())
            return HttpHelpers::mapAuthError(result.error());

        return successResponse(201, result.value());
    }

    crow::response handleLogin(const crow::request &req)
    {
        protocol::auth::LoginRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        auto result = auth_.login(dto.username, dto.password);
        if (!result.has_value())
            return HttpHelpers::mapAuthError(result.error());

        return successResponse(200, result.value());
    }

    crow::response handleRefresh(const crow::request &req)
    {
        // refresh-токен ВСЕГДА в теле, не в заголовке
        protocol::auth::RefreshRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        auto result = auth_.refresh(dto.refreshToken);
        if (!result.has_value())
            return HttpHelpers::errorResponse(401,
                                              protocol::ErrorCode::InvalidRefreshToken,
                                              "Refresh token is invalid or expired");

        return successResponse(200, result.value());
    }

    crow::response handleLogout(const crow::request &req)
    {
        protocol::auth::LogoutRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        // logout идемпотентен — даже если такого refresh уже нет, ответ тот же
        auth_.logout(dto.refreshToken);
        return crow::response(204);
    }

    crow::response handleLogoutAll(const crow::request &req)
    {
        // Этот endpoint требует ACCESS-токен (а не refresh) — из заголовка
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auth_.logoutAll(*userId);
        return crow::response(204);
    }

private:
    // ─────────────────────────────────────────────────────────────
    // Внутренние утилиты
    // ─────────────────────────────────────────────────────────────
    crow::response successResponse(int code, const AuthSuccess &s) const
    {
        protocol::auth::AuthSuccessResponse dto{
            .userId = s.userId,
            .accessToken = s.tokens.access,
            .refreshToken = s.tokens.refresh,
            .accessExpiresIn = s.accessTtl,
            .refreshExpiresIn = s.refreshTtl};
        return HttpHelpers::jsonResponse(code, dto);
    }
};
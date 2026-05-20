// Transport/HttpServer.hpp
#pragma once

#include <ctime>
#include <optional>
#include <string>

#include <crow/crow.h>
#include <nlohmann/json.hpp>

#include <Auth/AuthService.hpp>
#include <Protocol/Api.hpp>
#include <Protocol/Auth.hpp>
#include <Utils/Types.hpp>

// REST-слой аутентификации. Тонкий: парсит тело -> DTO, зовёт AuthService,
// раскладывает доменный результат в HTTP-ответ. Никакой бизнес-логики.
class HttpServer
{
public:
    HttpServer(crow::SimpleApp &app, AuthService &auth, std::time_t accessTtl, std::time_t refreshTtl)
        : app_(app), auth_(auth), accessTtl_(accessTtl), refreshTtl_(refreshTtl)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/health")
        ([]
        {
            return crow::response(200, R"({"status":"ok"})");
        });

        CROW_ROUTE(app_, "/api/v1/info")
        ([this]
        {
            return handleInfo();
        });

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
    const std::time_t accessTtl_;
    const std::time_t refreshTtl_;

    // ─────────────────────────────────────────────────────────────
    // ХЭНДЛЕРЫ
    // ─────────────────────────────────────────────────────────────

    crow::response handleInfo() const
    {
        protocol::api::InfoResponse dto{
            .serverName = "ZhiZha",
            .version = "0.1.0",
            .wsEndpoint = "/ws",
            .accessTtl = accessTtl_,
            .refreshTtl = refreshTtl_};
        return jsonResponse(200, dto);
    }

    crow::response handleRegister(const crow::request &req)
    {
        protocol::auth::RegisterRequest dto;
        if (!parseBody(req, dto))
            return errorResponse(400, protocol::api::ErrorCode::invalidFormat, "Malformed request body");

        auto result = auth_.registerUser(dto.username, dto.password, dto.displayName);
        if (!result.has_value())
            return mapAuthError(result.error());

        return successResponse(201, result.value());
    }

    crow::response handleLogin(const crow::request &req)
    {
        protocol::auth::LoginRequest dto;
        if (!parseBody(req, dto))
            return errorResponse(400, protocol::api::ErrorCode::invalidFormat, "Malformed request body");

        auto result = auth_.login(dto.username, dto.password);
        if (!result.has_value())
            return mapAuthError(result.error());

        return successResponse(200, result.value());
    }

    crow::response handleRefresh(const crow::request &req)
    {
        // refresh-токен ВСЕГДА в теле, не в заголовке
        protocol::auth::RefreshRequest dto;
        if (!parseBody(req, dto))
            return errorResponse(400, protocol::api::ErrorCode::invalidFormat, "Malformed request body");

        auto result = auth_.refresh(dto.refreshToken);
        if (!result.has_value())
            return errorResponse(401, protocol::api::ErrorCode::invalidRefreshToken,
                                 "Refresh token is invalid or expired");

        return successResponse(200, result.value());
    }

    crow::response handleLogout(const crow::request &req)
    {
        protocol::auth::LogoutRequest dto;
        if (!parseBody(req, dto))
            return errorResponse(400, protocol::api::ErrorCode::invalidFormat, "Malformed request body");

        // logout идемпотентен — даже если такого refresh уже нет, ответ тот же
        auth_.logout(dto.refreshToken);
        return crow::response(204); // No Content
    }

    crow::response handleLogoutAll(const crow::request &req)
    {
        // Этот endpoint требует ACCESS-токен (а не refresh) — из заголовка
        auto userId = requireAuth(req);
        if (!userId)
            return errorResponse(401, protocol::api::ErrorCode::unauthorized,
                                 "Invalid or missing access token");

        auth_.logoutAll(*userId);
        return crow::response(204);
    }

    // ─────────────────────────────────────────────────────────────
    // УТИЛИТЫ
    // ─────────────────────────────────────────────────────────────

    // Извлекает access-токен из заголовка Authorization: Bearer <token>
    // и проверяет его. Возвращает userId если всё ок.
    std::optional<UserId> requireAuth(const crow::request &req) const
    {
        std::string header = req.get_header_value("Authorization");
        if (header.empty() || !header.starts_with("Bearer "))
            return std::nullopt;

        std::string token = header.substr(7); // длина "Bearer "
        return auth_.validateAccess(token);
    }

    // Парсим тело запроса в DTO через nlohmann::json + сгенерированный from_json
    template <typename Dto>
    static bool parseBody(const crow::request &req, Dto &out)
    {
        try
        {
            nlohmann::json::parse(req.body).get_to(out);
            return true;
        }
        catch (const std::exception &)
        {
            return false; // битый JSON или несовпадение полей
        }
    }

    // Сериализует любой DTO с to_json в JSON-ответ.
    template <typename Dto>
    static crow::response jsonResponse(int code, const Dto &dto)
    {
        crow::response res(code, nlohmann::json(dto).dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }

    // Единый формат успешного ответа register/login/refresh.
    crow::response successResponse(int code, const AuthSuccess &s) const
    {
        protocol::auth::AuthSuccessResponse dto{
            .userId = s.userId,
            .accessToken = s.tokens.access,
            .refreshToken = s.tokens.refresh,
            .accessExpiresIn = s.accessTtl,
            .refreshExpiresIn = s.refreshTtl};
        return jsonResponse(code, dto);
    }

    // Единый формат ошибки.
    static crow::response errorResponse(int code, protocol::api::ErrorCode error, std::string message)
    {
        protocol::api::ErrorResponse dto{.code = error, .message = std::move(message)};
        return jsonResponse(code, dto);
    }

    // Перевод доменной ошибки AuthService в HTTP-ошибку.
    static crow::response mapAuthError(AuthError e)
    {
        switch (e)
        {
        case AuthError::UsernameTaken:
            return errorResponse(409, protocol::api::ErrorCode::usernameTaken, "Username is already taken");
        case AuthError::InvalidCredentials:
            return errorResponse(401, protocol::api::ErrorCode::invalidCredentials, "Invalid username or password");
        case AuthError::WeakPassword:
            return errorResponse(400, protocol::api::ErrorCode::weakPassword, "Password does not meet requirements");
        case AuthError::UsernameValidation:
            return errorResponse(400, protocol::api::ErrorCode::usernameValidation, "Username does not meet requirements");
        case AuthError::InvalidToken:
        case AuthError::TokenExpired:
        case AuthError::TokenReused:
            return errorResponse(401, protocol::api::ErrorCode::invalidToken, "Token is invalid or expired");
        }
        return errorResponse(500, protocol::api::ErrorCode::internalError, "Internal server error");
    }
};

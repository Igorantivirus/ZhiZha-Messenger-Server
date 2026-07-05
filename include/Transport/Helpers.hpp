#pragma once

#include <expected>
#include <optional>
#include <string>

#include <crow/crow.h>
#include <nlohmann/json.hpp>

#include <Protocol/Common/Error.hpp>
#include <Protocol/Common/Http.hpp>
#include <Protocol/Common/Types.hpp>

#include <Utils/RouteBinder.hpp>
#include <Utils/Types.hpp>

#include <AuthService/AuthService.hpp>
#include <ChatService/ChatService.hpp>

namespace transport
{

class Helpers
{
public:
    Helpers() = delete;

public:
    struct Empty
    {
    };

    template <protocol::Operation Op>
    using BodyType = std::conditional_t<std::is_void_v<typename Op::Request>, Empty, typename Op::Request>;

    template <protocol::Operation Op>
    struct HttpRequest
    {
        const crow::request &req;
        typename Op::PathParams path;
        typename Op::QueryParams query;
        BodyType<Op> body;
        std::optional<utils::UserId> userId;
    };

    template <protocol::Operation Op, typename Error>
    using HttpResponse = std::expected<typename Op::Response, Error>;

public: // Базовый запрос - ответ
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
            return false;
        }
    }
    template <typename Dto>
    static std::string dtoToString(Dto dto)
    {
        return nlohmann::json(dto).dump();
    }

    template <typename Dto>
    static crow::response jsonResponse(int code, Dto dto)
    {
        crow::response res(code, dtoToString(std::move(dto)));
        res.set_header("Content-Type", "application/json");
        return res;
    }

public: // handler
    template <typename Op, typename Self, typename MemberFunc>
    static void bindWithoutAuth(crow::SimpleApp &app, Self *self, MemberFunc func)
    {
        // Вызываем низкоуровневый bindHandler, передавая лямбду-обертку
        utils::RouteBinder::bindHandler<Op>(app, [self, func](const crow::request &req, typename Op::PathParams path, typename Op::QueryParams query) -> crow::response
        {
            HttpRequest<Op> httpReq{
                .req = req,
                .path = std::move(path),
                .query = std::move(query),
                .body = {},
                .userId = std::nullopt};
            if constexpr (!std::is_void_v<typename Op::Request>)
            {
                if (!parseBody(req, httpReq.body))
                    return invalidFormatResponse();
            }
            auto result = std::invoke(func, self, std::move(httpReq));
            if (!result.has_value())
                return mapError(result.error());
            if constexpr (std::is_void_v<typename Op::Response>)
                return crow::response(204);
            else
                return jsonResponse(200, result.value());
        });
    }

    // Универсальный биндер С авторизацией
    template <typename Op, typename Self, typename MemberFunc>
    static void bindWithAuth(crow::SimpleApp &app, Self *self, auth::AuthService &auth, MemberFunc func)
    {
        utils::RouteBinder::bindHandler<Op>(app, [self, &auth, func](const crow::request &req, typename Op::PathParams path, typename Op::QueryParams query) -> crow::response
        {
            auto userId = requireAuth(req, auth);
            if (!userId)
                return mapError(userId.error());
            HttpRequest<Op> httpReq{
                .req = req,
                .path = path,
                .query = query,
                .body = {},
                .userId = *userId};
            if constexpr (!std::is_void_v<typename Op::Request>)
            {
                if (!parseBody(req, httpReq.body))
                    return invalidFormatResponse();
            }
            auto result = std::invoke(func, self, std::move(httpReq));
            if (!result.has_value())
                return mapError(result.error());
            if constexpr (std::is_void_v<typename Op::Response>)
                return crow::response(204);
            else
                return jsonResponse(200, result.value());
        });
    }

public: // map'еры
    static protocol::ErrorCode mapErrorType(chat::ChatError e);
    static protocol::ErrorCode mapErrorType(auth::AuthError e);

    static crow::response mapError(chat::ChatError e);
    static crow::response mapError(auth::AuthError e);

public: // Шорткаты для частых ошибок
    static crow::response errorResponse(int code, protocol::ErrorCode error, std::string message);
    static crow::response unauthorizedResponse(std::string what = "Invalid or missing access token");
    static crow::response invalidFormatResponse(std::string what = "Malformed request body");
    static crow::response notFoundResponse(std::string what = "Resource not found");
    static crow::response forbiddenResponse(std::string what = "Forbidden");

public:
    static std::expected<utils::UserId, auth::AuthError> requireAuth(const crow::request &req, auth::AuthService &auth);
};

} // namespace transport
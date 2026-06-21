#pragma once

#include "ChatService/Types/ChatError.hpp"
#include <crow/crow.h>
#include <expected>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include <Auth/AuthService.hpp>
#include <ProtocolV1/Common/Error.hpp>
#include <ProtocolV1/Common/Http.hpp>
#include <ProtocolV1/Common/Types.hpp>

#include <Utils/RouteBinder.hpp>

// #include <Protocol/Api.hpp>
// #include <Protocol/Parsing.hpp>
// #include <Protocol/Types.hpp>

class HttpHelpers
{
public:
    HttpHelpers() = delete;

public:
    struct Empty
    {
    };

    template <typename Op>
    using BodyType = std::conditional_t<
        std::is_void_v<typename Op::Request>,
        Empty,
        typename Op::Request>;

    // ===== Универсальный HttpRequest =====
    template <typename Op>
    struct HttpRequest
    {
        const crow::request &req;
        typename Op::PathParams path;
        typename Op::QueryParams query;
        BodyType<Op> body;
        std::optional<protocol::UserId> userId;
    };

    template <typename Op, typename Error>
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
    static crow::response jsonResponse(int code, const Dto &dto)
    {
        crow::response res(code, nlohmann::json(dto).dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    static crow::response errorResponse(int code, protocol::ErrorCode error, std::string message)
    {
        protocol::ErrorResponse dto{.code = error, .message = std::move(message)};
        return jsonResponse(code, dto);
    }

public: // Шорткаты для частых ошибок
    static crow::response unauthorizedResponse()
    {
        return errorResponse(401, protocol::ErrorCode::Unauthorized, "Invalid or missing access token");
    }
    static crow::response invalidFormatResponse()
    {
        return errorResponse(400, protocol::ErrorCode::InvalidFormat, "Malformed request body");
    }
    static crow::response notFoundResponse(std::string what = "Resource not found")
    {
        return errorResponse(404, protocol::ErrorCode::NotFound, std::move(what));
    }
    static crow::response forbiddenResponse(std::string what = "Forbidden")
    {
        return errorResponse(403, protocol::ErrorCode::Forbidden, std::move(what));
    }

public:
    static std::optional<protocol::UserId> requireAuth(const crow::request &req, AuthService &auth)
    {
        std::string header = req.get_header_value("Authorization");
        if (header.empty() || !header.starts_with("Bearer "))
            return std::nullopt;
        return auth.validateAccess(header.substr(7));
    }

public: // handler
    template <typename Op, typename Self, typename MemberFunc>
    static void bindWithoutAuth(crow::SimpleApp &app, Self *self, MemberFunc func)
    {
        // Вызываем низкоуровневый bindHandler, передавая лямбду-обертку
        utils::bindHandler<Op>(app, [self, func](const crow::request &req, typename Op::PathParams path, typename Op::QueryParams query) -> crow::response
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
    static void bindWithAuth(crow::SimpleApp &app, Self *self, AuthService &auth, MemberFunc func)
    {
        utils::bindHandler<Op>(app, [self, &auth, func](const crow::request &req, typename Op::PathParams path, typename Op::QueryParams query) -> crow::response
        {
            auto userId = requireAuth(req, auth);
            if (!userId)
                return unauthorizedResponse();
            HttpRequest<Op> httpReq{
                .req = req,
                .path = path,
                .query = query,
                .body = {},
                .userId = userId};
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
    static crow::response mapError(ChatError e)
    {
        const ChatErrorInfo info = mapChatErrorInfo(e);
        return errorResponse(info.httpStatus, info.code, std::string(info.message));
    }
    static crow::response mapError(AuthError e)
    {
        using protocol::ErrorCode;
        switch (e)
        {
        case AuthError::UsernameTaken:
            return errorResponse(409, ErrorCode::UsernameTaken, "Username is already taken");
        case AuthError::InvalidCredentials:
            return errorResponse(401, ErrorCode::InvalidCredentials, "Invalid username or password");
        case AuthError::WeakPassword:
            return errorResponse(400, ErrorCode::WeakPassword, "Password does not meet requirements");
        case AuthError::UsernameValidation:
            return errorResponse(400, ErrorCode::UsernameValidation, "Username does not meet requirements");
        case AuthError::InvalidToken:
        case AuthError::TokenExpired:
        case AuthError::TokenReused:
            return errorResponse(401, ErrorCode::InvalidToken, "Token is invalid or expired");
        }
        return errorResponse(500, ErrorCode::InternalError, "Internal server error");
    }
    // static protocol::ws::ErrorMessage mapErrorWs(ChatError e)
    // {
    //     const ChatErrorInfo info = mapChatErrorInfo(e);
    //     return protocol::ws::ErrorMessage{.code = info.code, .message = std::string(info.message)};
    // }

private:
    struct ChatErrorInfo
    {
        int httpStatus;           // только для HTTP-ответа
        protocol::ErrorCode code; // общий код для HTTP и WS
        std::string_view message; // человекочитаемый текст
    };

    // Единственная таблица соответствия ChatError -> код/текст/HTTP-статус.
    static ChatErrorInfo mapChatErrorInfo(ChatError e)
    {
        using protocol::ErrorCode;
        switch (e)
        {
        case ChatError::NotAMember:
            return {403, ErrorCode::NotAMember, "You are not a member of this room"};
        case ChatError::WriteForbidden:
            return {403, ErrorCode::WriteForbidden, "You don't have permission to write in this room"};
        case ChatError::PermissionError:
            return {403, ErrorCode::Forbidden, "You don't have permission for this action"};
        case ChatError::EmptyMessage:
            return {400, ErrorCode::EmptyMessage, "Message cannot be empty"};
        case ChatError::MessageTooLong:
            return {413, ErrorCode::MessageTooLong, "Message exceeds maximum allowed length"};
        case ChatError::RoomNotFound:
            return {404, ErrorCode::RoomNotFound, "Room not found"};
        case ChatError::InvalidDirectRoom:
            return {400, ErrorCode::InvalidDirectRoom, "Direct room must have exactly one invited user"};
        case ChatError::EmptyRoomName:
            return {400, ErrorCode::EmptyRoomName, "Room name cannot be empty"};
        case ChatError::MemberAlready:
            return {409, ErrorCode::MemberAlready, "User is already a member of this room"};
        }
        return {500, ErrorCode::InternalError, "Internal server error"};
    }
};
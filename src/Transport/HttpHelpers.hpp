#pragma once

#include "ChatService/Types/ChatError.hpp"
#include <crow/crow.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include <Auth/AuthService.hpp>
#include <Protocol/Api.hpp>
#include <Protocol/Parsing.hpp>
#include <Protocol/Types.hpp>

class HttpHelpers
{
public:
    HttpHelpers() = delete;

    // Извлекает access-токен из "Authorization: Bearer <token>" и валидирует
    static std::optional<protocol::UserId> requireAuth(const crow::request &req,
                                                       AuthService &auth)
    {
        std::string header = req.get_header_value("Authorization");
        if (header.empty() || !header.starts_with("Bearer "))
            return std::nullopt;
        return auth.validateAccess(header.substr(7));
    }

    // Парсит тело запроса в DTO через nlohmann::json
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

    // Сериализует DTO в JSON-ответ
    template <typename Dto>
    static crow::response jsonResponse(int code, const Dto &dto)
    {
        crow::response res(code, nlohmann::json(dto).dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }

    // Единый формат ошибки
    static crow::response errorResponse(int code,
                                        protocol::ErrorCode error,
                                        std::string message)
    {
        protocol::api::ErrorResponse dto{
            .code = error,
            .message = std::move(message)};
        return jsonResponse(code, dto);
    }

    // Шорткаты для частых ошибок
    static crow::response unauthorizedResponse()
    {
        return errorResponse(401, protocol::ErrorCode::Unauthorized,
                             "Invalid or missing access token");
    }

    static crow::response invalidFormatResponse()
    {
        return errorResponse(400, protocol::ErrorCode::InvalidFormat,
                             "Malformed request body");
    }

    static crow::response notFoundResponse(std::string what = "Resource not found")
    {
        return errorResponse(404, protocol::ErrorCode::NotFound, std::move(what));
    }

    static crow::response forbiddenResponse(std::string what = "Forbidden")
    {
        return errorResponse(403, protocol::ErrorCode::Forbidden, std::move(what));
    }

    // Доменная ошибка чата -> HTTP-код + protocol::ErrorCode.
    static crow::response mapChatError(ChatError e)
    {
        using protocol::ErrorCode;
        switch (e)
        {
        case ChatError::NotAMember:
            return errorResponse(403, ErrorCode::NotAMember, "You are not a member of this room");
        case ChatError::WriteForbidden:
            return errorResponse(403, ErrorCode::WriteForbidden, "You don't have permission to write in this room");
        case ChatError::PermissionError:
            return errorResponse(403, ErrorCode::Forbidden, "You don't have permission for this action");
        case ChatError::EmptyMessage:
            return errorResponse(400, ErrorCode::EmptyMessage, "Message cannot be empty");
        case ChatError::MessageTooLong:
            return errorResponse(413, ErrorCode::MessageTooLong, "Message exceeds maximum allowed length");
        case ChatError::RoomNotFound:
            return errorResponse(404, ErrorCode::RoomNotFound, "Room not found");
        case ChatError::InvalidDirectRoom:
            return errorResponse(400, ErrorCode::InvalidDirectRoom, "Direct room must have exactly one invited user");
        case ChatError::EmptyRoomName:
            return errorResponse(400, ErrorCode::EmptyRoomName, "Room name cannot be empty");
        case ChatError::MemberAlready:
            return errorResponse(409, ErrorCode::MemberAlready, "User is already a member of this room");
        }
        return errorResponse(500, ErrorCode::InternalError, "Internal server error");
    }

    // Доменная ошибка аутентификации -> HTTP-код + protocol::ErrorCode.
    static crow::response mapAuthError(AuthError e)
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
};
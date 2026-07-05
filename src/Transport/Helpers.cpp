#include <Transport/Helpers.hpp>

#include <string>
#include <string_view>

#include <Protocol/Ws/EventParsing.hpp>

namespace
{

// Единая запись о том, как переводить доменную ошибку в ответ:
// HTTP-статус (нужен только для REST), общий ErrorCode (для HTTP и WS),
// и человекочитаемый текст-заглушка.
struct ErrorInfo
{
    int httpStatus;
    protocol::ErrorCode code;
    std::string_view message;
};

ErrorInfo mapChatErrorInfo(const chat::ChatError e)
{
    using protocol::ErrorCode;
    switch (e)
    {
    case chat::ChatError::NotAMember:
        return {403, ErrorCode::NotAMember, "You are not a member of this room"};
    case chat::ChatError::WriteForbidden:
        return {403, ErrorCode::WriteForbidden, "You don't have permission to write in this room"};
    case chat::ChatError::PermissionError:
        return {403, ErrorCode::Forbidden, "You don't have permission for this action"};
    case chat::ChatError::EmptyMessage:
        return {400, ErrorCode::EmptyMessage, "Message cannot be empty"};
    case chat::ChatError::MessageTooLong:
        return {413, ErrorCode::MessageTooLong, "Message exceeds maximum allowed length"};
    case chat::ChatError::RoomNotFound:
        return {404, ErrorCode::RoomNotFound, "Room not found"};
    case chat::ChatError::InvalidDirectRoom:
        return {400, ErrorCode::InvalidDirectRoom, "Direct room must have exactly one invited user"};
    case chat::ChatError::InvalidRoomKind:
        // В protocol::ErrorCode нет точного аналога — мапим на общий InvalidFormat.
        return {400, ErrorCode::InvalidFormat, "Invalid room kind for this operation"};
    case chat::ChatError::EmptyRoomName:
        return {400, ErrorCode::EmptyRoomName, "Room name cannot be empty"};
    case chat::ChatError::MemberAlready:
        return {409, ErrorCode::MemberAlready, "User is already a member of this room"};
    case chat::ChatError::UserNotFound:
        return {404, ErrorCode::NotFound, "User not found"};
    }
    return {500, ErrorCode::InternalError, "Internal server error"};
}

ErrorInfo mapAuthErrorInfo(const auth::AuthError e)
{
    using protocol::ErrorCode;
    switch (e)
    {
    case auth::AuthError::UsernameTaken:
        return {409, ErrorCode::UsernameTaken, "Username is already taken"};
    case auth::AuthError::InvalidCredentials:
        return {401, ErrorCode::InvalidCredentials, "Invalid username or password"};
    case auth::AuthError::WeakPassword:
        return {400, ErrorCode::WeakPassword, "Password does not meet requirements"};
    case auth::AuthError::UsernameValidation:
        return {400, ErrorCode::UsernameValidation, "Username does not meet requirements"};
    case auth::AuthError::InvalidToken:
        return {401, ErrorCode::InvalidToken, "Token is invalid"};
    case auth::AuthError::TokenExpired:
        // Для клиента семантически то же, что InvalidToken — нужно заново логиниться.
        return {401, ErrorCode::InvalidToken, "Token has expired"};
    case auth::AuthError::TokenReused:
        // Refresh одноразовый — повторное использование сигнализирует об утечке.
        return {401, ErrorCode::InvalidRefreshToken, "Refresh token has already been used"};
    case auth::AuthError::NoSendAccess:
        return {401, ErrorCode::Unauthorized, "Invalid or missing access token"};
    case auth::AuthError::UserNotFound:
        return {404, ErrorCode::NotFound, "User not found"};
    }
    return {500, ErrorCode::InternalError, "Internal server error"};
}

} // namespace

namespace transport
{

protocol::ErrorCode Helpers::mapErrorType(chat::ChatError e)
{
    return mapChatErrorInfo(e).code;
}
protocol::ErrorCode Helpers::mapErrorType(auth::AuthError e)
{
    return mapAuthErrorInfo(e).code;
}

crow::response Helpers::mapError(chat::ChatError e)
{
    const ErrorInfo info = mapChatErrorInfo(e);
    return errorResponse(info.httpStatus, info.code, std::string(info.message));
}
crow::response Helpers::mapError(auth::AuthError e)
{
    const ErrorInfo info = mapAuthErrorInfo(e);
    return errorResponse(info.httpStatus, info.code, std::string(info.message));
}

crow::response Helpers::errorResponse(int code, protocol::ErrorCode error, std::string message)
{
    protocol::ErrorResponse dto{.code = error, .message = std::move(message)};
    return jsonResponse(code, dto);
}

crow::response Helpers::unauthorizedResponse(std::string what)
{
    return errorResponse(401, protocol::ErrorCode::Unauthorized, std::move(what));
}
crow::response Helpers::invalidFormatResponse(std::string what)
{
    return errorResponse(400, protocol::ErrorCode::InvalidFormat, std::move(what));
}
crow::response Helpers::notFoundResponse(std::string what)
{
    return errorResponse(404, protocol::ErrorCode::NotFound, std::move(what));
}
crow::response Helpers::forbiddenResponse(std::string what)
{
    return errorResponse(403, protocol::ErrorCode::Forbidden, std::move(what));
}



std::expected<utils::UserId, auth::AuthError> Helpers::requireAuth(const crow::request &req, auth::AuthService &auth)
{
    const std::string header = req.get_header_value("Authorization");
    constexpr std::string_view prefix = "Bearer ";
    if (header.size() <= prefix.size() || !header.starts_with(prefix))
        return std::unexpected(auth::AuthError::NoSendAccess);

    // validateAccess сам уже возвращает expected<UserId, AuthError> — пробрасываем.
    return auth.validateAccess(header.substr(prefix.size()));
}

} // namespace transport

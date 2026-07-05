#pragma once

#include <Protocol/Common/Http.hpp>
#include <Protocol/Dto/Auth.hpp>

namespace protocol::auth
{

// Операция регистрации
struct RegisterOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "auth/register";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = dto::RegisterRequestDto;
    using Response = dto::AuthSuccessResponseDto;
};

// Операция входа по логику и паролю
struct LoginOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "auth/login";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = dto::LoginRequestDto;
    using Response = dto::AuthSuccessResponseDto;
};

// Операция обновления токенов
struct RefreshOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "auth/refresh";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = dto::RefreshRequestDto;
    using Response = dto::AuthSuccessResponseDto;
};

// Операция завершеения сессии
struct LogoutOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "auth/logout";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = dto::LogoutRequestDto;
    using Response = void;
};

// Операция завершения сессии со всех устройств
struct LogoutAllOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "auth/logout-all";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = void;
};

} // namespace protocol::auth
#pragma once

#include <string>

#include <Protocol/Common/Http.hpp>
#include <Protocol/Common/Types.hpp>
#include <Protocol/Dto/Users.hpp>

namespace protocol::users
{

// Операция поиска пользователей по запросу
struct LoopUsersOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "users/loop";

    struct PathParams
    {
    };
    struct QueryParams
    {
        std::string query; // Запрос имени пользователя
        unsigned limit;    // Предел пользователей для получения
    };

    using Request = void;
    using Response = dto::LoopUsersResponseDto;
};

// Операция изменения данных о себе
struct ChangeMeOperation
{
    static constexpr HttpMethod method = HttpMethod::put;
    static constexpr Path path = API "users/me";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = dto::ChangeMeRequestDto;
    using Response = void;
};

// Операция получения информации о себе
struct GetMeOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "users/me";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = dto::UserResponseDto;
};

// Операция получения информации о пользователе
struct GetUserOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "users/<uint>";

    struct PathParams
    {
        UserId userId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = dto::UserResponseDto;
};

} // namespace protocol::users

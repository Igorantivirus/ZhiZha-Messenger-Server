#pragma once

#include <ProtocolV1/Common/Http.hpp>
#include <ProtocolV1/Dto/Api.hpp>

namespace protocol::api
{

// Операция проверки состосния сервера
struct ServerHealthOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "health";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = dto::ServerHealthResponseDto;
};

// Операция получения информации о сервере
struct ServerInfoOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "info";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = dto::ServerInfoResponseDto;
};

} // namespace protocol::api
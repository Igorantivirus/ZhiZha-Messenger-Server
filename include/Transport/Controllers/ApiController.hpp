#pragma once

#include <Protocol/Operations/Api.hpp>

#include <Transport/Helpers.hpp>

namespace transport
{

class ApiController
{
public:
    ApiController(
        crow::SimpleApp &app,
        const std::time_t accessTtl,
        const std::time_t refreshTtl,
        const unsigned maxMessageSize);

    void registerRoutes();

private:
    crow::SimpleApp &app_;
    const std::time_t accessTtl_;
    const std::time_t refreshTtl_;
    const std::int64_t maxMessageSize_;

private:
    using Health = protocol::api::ServerHealthOperation;
    using Info = protocol::api::ServerInfoOperation;

private:
    Helpers::HttpResponse<Health, chat::ChatError> handleHealth(Helpers::HttpRequest<Health> req);
    Helpers::HttpResponse<Info, chat::ChatError> handleInfo(Helpers::HttpRequest<Info> req);
};

} // namespace transport
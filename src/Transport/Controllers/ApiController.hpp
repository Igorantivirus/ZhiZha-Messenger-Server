#pragma once

#include <ProtocolV1/Operations/Api.hpp>
#include <Transport/HttpHelpers.hpp>

class ApiController
{
private:
    using Health = protocol::api::ServerHealthOperation;
    using Info = protocol::api::ServerInfoOperation;

public:
    ApiController(crow::SimpleApp &app,
                  std::time_t accessTtl,
                  std::time_t refreshTtl,
                  const std::int64_t maxMessageSize)
        : app_(app),
          accessTtl_(accessTtl),
          refreshTtl_(refreshTtl),
          maxMessageSize_(maxMessageSize)
    {
    }

    void registerRoutes()
    {
        HttpHelpers::bindWithoutAuth<Health>(app_, this, &ApiController::handleHalth);
        HttpHelpers::bindWithoutAuth<Info>(app_, this, &ApiController::handleInfo);
    }

private:
    crow::SimpleApp &app_;
    const std::time_t accessTtl_;
    const std::time_t refreshTtl_;
    const std::int64_t maxMessageSize_;

private:
    HttpHelpers::HttpResponse<Health, ChatError> handleHalth(HttpHelpers::HttpRequest<Health> req)
    {
        return Health::Response{.status = protocol::data::ServerStatus::Ok};
    }
    HttpHelpers::HttpResponse<Info, ChatError> handleInfo(HttpHelpers::HttpRequest<Info> req)
    {
        Info::Response dto{
            .serverName = "ZhiZha",
            .version = "0.1.0",
            .wsEndpoint = "/ws",
            .accessTtl = accessTtl_,
            .refreshTtl = refreshTtl_,
            .maxMessageSize = maxMessageSize_};
        return dto;
    }
};
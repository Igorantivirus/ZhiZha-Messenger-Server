#pragma once

#include <crow/crow.h>

#include <Utils/BindMethod.hpp>

#include <Protocol/Api.hpp>

#include <Transport/HttpHelpers.hpp>

class ApiController
{
public:
    ApiController(crow::SimpleApp &app, std::time_t accessTtl, std::time_t refreshTtl)
        : app_(app), accessTtl_(accessTtl), refreshTtl_(refreshTtl)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/health").methods("GET"_method)(utils::bindMethod(this, &ApiController::handleHealth));
        CROW_ROUTE(app_, "/api/v1/info").methods("GET"_method)(utils::bindMethod(this, &ApiController::handleInfo));
    }

private:
    crow::SimpleApp &app_;
    const std::time_t accessTtl_;
    const std::time_t refreshTtl_;

private:
    crow::response handleHealth() const
    {
        protocol::api::HealthResponse dto{.status = protocol::api::ServerStatus::Ok};
        return HttpHelpers::jsonResponse(200, dto);
    }

    crow::response handleInfo() const
    {
        protocol::api::InfoResponse dto{
            .serverName = "ZhiZha",
            .version = "0.1.0",
            .wsEndpoint = "/ws",
            .accessTtl = accessTtl_,
            .refreshTtl = refreshTtl_};
        return HttpHelpers::jsonResponse(200, dto);
    }
};
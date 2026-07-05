#include "Protocol/Data/Server.hpp"
#include "Transport/Helpers.hpp"
#include <Transport/Controllers/ApiController.hpp>

namespace transport
{

ApiController::ApiController(
    crow::SimpleApp &app,
    const std::time_t accessTtl,
    const std::time_t refreshTtl,
    const unsigned maxMessageSize)
    : app_(app),
      accessTtl_(accessTtl),
      refreshTtl_(refreshTtl),
      maxMessageSize_(maxMessageSize)
{
}

void ApiController::registerRoutes()
{
    Helpers::bindWithoutAuth<ApiController::Health>(app_, this, &ApiController::handleHealth);
    Helpers::bindWithoutAuth<ApiController::Info>(app_, this, &ApiController::handleInfo);
}

Helpers::HttpResponse<ApiController::Health, chat::ChatError> ApiController::handleHealth(Helpers::HttpRequest<ApiController::Health> req)
{
    ApiController::Health::Response resp;
    resp.status = protocol::data::ServerStatus::Ok;
    return resp;
}
Helpers::HttpResponse<ApiController::Info, chat::ChatError> ApiController::handleInfo(Helpers::HttpRequest<ApiController::Info> req)
{
    ApiController::Info::Response resp;
    resp.accessTtl = accessTtl_;
    resp.refreshTtl = refreshTtl_;
    resp.maxMessageSize = maxMessageSize_;
    resp.version = "0.2.0";
    resp.wsEndpoint = "/ws";
    resp.serverName = "ZhiZha-Server";
    return resp;
}

} // namespace transport
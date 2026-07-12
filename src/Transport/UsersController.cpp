#include "Protocol/Data/Users.hpp"
#include "Transport/Helpers.hpp"
#include <Transport/Controllers/UsersController.hpp>

namespace transport
{

UsersController::UsersController(crow::SimpleApp &app, auth::AuthService &auth, auth::UserQueryService &userQuery)
    : app_(app), auth_(auth), userQuery_(userQuery)
{
}
void UsersController::registerRoutes()
{
    Helpers::bindWithAuth<LoopUsers>(app_, this, auth_, &UsersController::handleLoopUsers);
    Helpers::bindWithAuth<ChangeMe>(app_, this, auth_, &UsersController::handleChangeMe);
    Helpers::bindWithAuth<GetMe>(app_, this, auth_, &UsersController::handleGetMe);
    Helpers::bindWithAuth<GetUser>(app_, this, auth_, &UsersController::handleGetUser);
}
Helpers::HttpResponse<UsersController::LoopUsers, auth::AuthError> UsersController::handleLoopUsers(Helpers::HttpRequest<LoopUsers> req)
{
    auto res = userQuery_.searchUsers(req.query.query, req.query.limit);

    LoopUsers::Response resp;
    for (auto &&user : res)
        resp.users[user.id] = protocol::data::UserDisplayInfo{.displayName = user.displayName};

    return resp;
}
Helpers::HttpResponse<UsersController::ChangeMe, auth::AuthError> UsersController::handleChangeMe(Helpers::HttpRequest<ChangeMe> req)
{
    auth::UserUpdate upd;
    upd.username = req.body.info.username;
    upd.displayName = req.body.info.displayname;
    upd.country = req.body.info.country;
    upd.birthDate = req.body.info.birthDate;
    auto res = auth_.updateUser(*req.userId, std::move(upd));
    if (!res)
        return std::unexpected(res.error());
    return {};
}
Helpers::HttpResponse<UsersController::GetMe, auth::AuthError> UsersController::handleGetMe(Helpers::HttpRequest<GetMe> req)
{
    auto res = userQuery_.getUser(*req.userId);
    if (!res)
        return std::unexpected(auth::AuthError::UserNotFound);
    GetMe::Response resp;
    resp.info.userId = std::move(res->id);
    resp.info.displayInfo.displayName = std::move(res->displayName);
    resp.info.additionalInfo.username = std::move(res->username);
    resp.info.additionalInfo.birthDate = std::move(res->birthDate);
    resp.info.additionalInfo.country = std::move(res->country);
    resp.info.additionalInfo.registerTime = std::move(res->registerTime);
    return resp;
}
Helpers::HttpResponse<UsersController::GetUser, auth::AuthError> UsersController::handleGetUser(Helpers::HttpRequest<GetUser> req)
{
    auto res = userQuery_.getUser(*req.userId);
    if (!res)
        return std::unexpected(auth::AuthError::UserNotFound);

    GetMe::Response resp;
    resp.info.userId = std::move(res->id);
    resp.info.displayInfo.displayName = std::move(res->displayName);
    resp.info.additionalInfo.username = std::move(res->username);
    resp.info.additionalInfo.birthDate = std::move(res->birthDate);
    resp.info.additionalInfo.country = std::move(res->country);
    resp.info.additionalInfo.registerTime = std::move(res->registerTime);
    return resp;
}

} // namespace transport
#pragma once

#include "Protocol/ErrorCode.hpp"
#include "Protocol/Types.hpp"
#include <crow/crow.h>

#include <Utils/BindMethod.hpp>

#include <Auth/AuthService.hpp>
#include <Auth/Interfaces/IUserRepository.hpp>
#include <Protocol/Users.hpp>
#include <Transport/HttpHelpers.hpp>
#include <ranges>
#include <unordered_map>

class UsersController
{
public:
    UsersController(crow::SimpleApp &app,
                    AuthService &auth,
                    IUserRepository &userRepo)
        : app_(app), auth_(auth), userRepo_(userRepo)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/me")(utils::bindMethod(this, &UsersController::handleGetMe));
        CROW_ROUTE(app_, "/api/v1/users/loop").methods("GET"_method)(utils::bindMethod(this, &UsersController::handleLoop));
        CROW_ROUTE(app_, "/api/v1/users/<uint>")(utils::bindMethod(this, &UsersController::handleGetUser));
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;
    IUserRepository &userRepo_;

private:
    crow::response handleLoop(const crow::request &req)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto query = req.url_params.get("query"); // const char* или nullptr
        auto limit = req.url_params.get("limit");

        if (!query || !limit)
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::MissingParams, "Missing params: query or limit.");

        auto users = userRepo_.findUsersByQuery(query, std::atoi(limit));
        if (!users)
            return HttpHelpers::errorResponse(500, protocol::ErrorCode::InternalError, "Error of generate users list.");

        protocol::users::UsersLoopByExampleResponse resp;
        resp.users = users.value() | std::views::transform([](const User &user) -> std::pair<protocol::UserId, protocol::users::UserDisplayInfo>
        {
            return std::pair<protocol::UserId, protocol::users::UserDisplayInfo>{
                user.id, protocol::users::UserDisplayInfo{.username = user.username, .displayname = user.displayeName}
            };
        }) | std::ranges::to<std::unordered_map<protocol::UserId, protocol::users::UserDisplayInfo>>();
        return HttpHelpers::jsonResponse(200, resp);
    }

    crow::response handleGetMe(const crow::request &req)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto user = userRepo_.findUserById(*userId);
        if (!user)
            return HttpHelpers::notFoundResponse("User not found");

        protocol::users::MeResponse dto{
            .userId = user->id,
            .username = user->username,
            .displayname = user->displayeName,
            .registerTime = user->registerTime};
        return HttpHelpers::jsonResponse(200, dto);
    }

    crow::response handleGetUser(const crow::request &req, std::uint64_t id)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto user = userRepo_.findUserById(id);
        if (!user)
            return HttpHelpers::notFoundResponse("User not found");

        protocol::users::UserResponse dto{
            .userId = user->id,
            .username = user->username,
            .displayname = user->displayeName};
        return HttpHelpers::jsonResponse(200, dto);
    }
};
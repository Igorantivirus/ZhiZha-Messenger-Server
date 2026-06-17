#pragma once

#include "Protocol/ErrorCode.hpp"
#include "Protocol/Types.hpp"
#include "Utils/QueryParamsHelper.hpp"
#include <crow/crow.h>

#include <Utils/BindMethod.hpp>

#include <Auth/AuthService.hpp>
#include <Auth/Interfaces/IUserRepository.hpp>
#include <Protocol/Users.hpp>
#include <Transport/HttpHelpers.hpp>
#include <optional>
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
        CROW_ROUTE(app_, "/api/v1/users/me").methods("GET"_method)(utils::bindMethod(this, &UsersController::handleGetMe));
        CROW_ROUTE(app_, "/api/v1/users/loop").methods("GET"_method)(utils::bindMethod(this, &UsersController::handleLoop));
        CROW_ROUTE(app_, "/api/v1/users/<uint>").methods("GET"_method)(utils::bindMethod(this, &UsersController::handleGetUser));
        CROW_ROUTE(app_, "/api/v1/users/<uint>").methods("PUT"_method)(utils::bindMethod(this, &UsersController::handleUpdateUser));
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

        std::optional<std::string> query = utils::parseQuery<std::string>(req.url_params.get("query"));
        std::optional<unsigned> limit = utils::parseQuery<unsigned>(req.url_params.get("limit"));

        if (!query || !limit)
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::MissingParams, "Missing params: query or limit.");

        auto users = userRepo_.findUsersByQuery(std::move(query.value()), std::move(limit.value()));

        protocol::users::UsersLoopByExampleResponse resp;
        resp.users = users | std::views::transform([](const User &user) -> std::pair<protocol::UserId, protocol::users::UserDisplayInfo>
        {
            return std::pair<protocol::UserId, protocol::users::UserDisplayInfo>{
                user.id, protocol::users::UserDisplayInfo{
                    .info = {
                        .username = user.username,
                        .displayname = user.displayeName,
                        .birthDate = user.birthDate,
                        .country = user.country},
                    .registerTime = user.registerTime}
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
            .display = {
                .info = {
                    .username = user->username,
                    .displayname = user->displayeName,
                    .birthDate = user->birthDate,
                    .country = user->country},
                .registerTime = user->registerTime}};
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
            .display = {
                .info = {
                    .username = user->username,
                    .displayname = user->displayeName,
                    .birthDate = user->birthDate,
                    .country = user->country},
                .registerTime = user->registerTime}};
        return HttpHelpers::jsonResponse(200, dto);
    }

    crow::response handleUpdateUser(const crow::request &req, std::uint64_t targetId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        // Редактировать можно только свой профиль.
        if (static_cast<std::uint64_t>(*userId) != targetId)
            return HttpHelpers::forbiddenResponse("You can only edit your own profile");

        protocol::users::ChangeUserRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        // Проверка username (валидность + уникальность) и запись — в AuthService.
        if (auto error = auth_.updateUser(*userId, dto.newInfo))
            return HttpHelpers::mapAuthError(error.value());

        return crow::response(204);
    }
};
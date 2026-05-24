#pragma once

#include <crow/crow.h>

#include <Auth/AuthService.hpp>
#include <Auth/Interfaces/IUserRepository.hpp>
#include <Protocol/Users.hpp>
#include <Transport/HttpHelpers.hpp>

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
        CROW_ROUTE(app_, "/api/v1/me")
        ([this](const crow::request &req)
        {
            return handleGetMe(req);
        });

        CROW_ROUTE(app_, "/api/v1/users/<uint>")
        ([this](const crow::request &req, std::uint64_t id)
        {
            return handleGetUser(req, id);
        });
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;
    IUserRepository &userRepo_;

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
#pragma once

#include <crow/crow.h>

#include <Auth/AuthService.hpp>

class HttpServer
{
public:
    HttpServer(crow::SimpleApp &app, AuthService &auth)
        : app_(app), auth_(auth)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/info")
        ([this]
        {
            return handleInfo();
        });
        
        CROW_ROUTE(app_, "/api/v1/auth/register").methods("POST"_method)([this](const crow::request &req)
        {
            return handleRegister(req);
        });
        CROW_ROUTE(app_, "/api/v1/auth/login").methods("POST"_method)([this](const crow::request &req)
        {
            return handleLogin(req);
        });
        CROW_ROUTE(app_, "/api/v1/auth/refresh").methods("POST"_method)([this](const crow::request &req)
        {
            return handleRefresh(req);
        });
        CROW_ROUTE(app_, "/api/v1/auth/logout").methods("POST"_method)([this](const crow::request &req)
        {
            return handleLogout(req);
        });
        CROW_ROUTE(app_, "/api/v1/auth/logout-all").methods("POST"_method)([this](const crow::request &req)
        {
            return handleLogoutAll(req);
        });
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;

private:
    crow::response handleRegister(const crow::request &req)
    {
        return {};
    }
    crow::response handleLogin(const crow::request &req)
    {
        return {};
    }
    crow::response handleRefresh(const crow::request &req)
    {
        return {};
    }
    crow::response handleLogout(const crow::request &req)
    {
        return {};
    }
    crow::response handleLogoutAll(const crow::request &req)
    {
        return {};
    }

    crow::response handleInfo()
    {
        return {};
    }
};
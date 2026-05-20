#pragma once

#include <crow/crow.h>

#include <Transport/HttpServer.hpp>
#include <Transport/WsServer.hpp>

class ServerApplication
{
public:
    ServerApplication()
    {
        httpServerBind();
        wsServerBind();
    }

    void run(const std::uint16_t port)
    {
        app.port(port).multithreaded().run();
    }

private:
    crow::SimpleApp app;

    HttpServer httpServer;
    WsServer wsServer;

private:

    void httpServerBind()
    {

    }

    void wsServerBind()
    {

    }

};
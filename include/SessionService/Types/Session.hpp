#pragma once

#include <crow/crow.h>

#include <Utils/Types.hpp>

namespace session
{

struct Session
{
    utils::UserId userId;
    crow::websocket::connection *connection;
};

} // namespace session
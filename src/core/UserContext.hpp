#pragma once

#include <atomic>
#include <chrono>
#include <set>
#include <string>

#include <crow/websocket.h>

#include "core/Types.hpp"

struct UserContext
{
    crow::websocket::connection* connection = nullptr;
    std::chrono::steady_clock::time_point connectionTime = std::chrono::steady_clock::now();

    IDType userId = 0;
    std::string username;
    std::string password;
    std::string publicKey;
    std::set<IDType> roomIds;
    std::atomic_bool authorized = false;
    std::atomic_bool closing = false;
};

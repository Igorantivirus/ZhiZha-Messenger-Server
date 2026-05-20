#pragma once

#include <ctime>
#include <string>
#include <utility>

struct ServerConfig
{
    std::time_t accessTtl_;
    std::time_t refreshTtl_;

    std::string dataBaseFileName;
    std::pair<std::size_t, std::size_t> userNamePasswordLengthLimit;
    std::pair<std::size_t, std::size_t> passwordPasswordLengthLimit;
    std::pair<std::size_t, std::size_t> roomNamePasswordLengthLimit;
    std::pair<std::size_t, std::size_t> displayNamePasswordLengthLimit;

    // other settings
};
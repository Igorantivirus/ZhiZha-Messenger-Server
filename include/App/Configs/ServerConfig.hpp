#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#include "ValidationConfig.hpp"

namespace app
{

struct ServerConfig
{
    std::uint16_t port;
    std::time_t accessTtl;
    std::time_t refreshTtl;
    std::string databaseFile;
    ValidationConfig validation;
};

} // namespace app
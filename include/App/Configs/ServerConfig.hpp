#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#include "TlsConfig.hpp"
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
    TlsConfig tls;
};

} // namespace app
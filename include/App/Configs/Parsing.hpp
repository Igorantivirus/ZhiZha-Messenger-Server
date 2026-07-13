#pragma once

#include <Protocol/Common/Parsing.hpp>
#include <Utils/StringRule.hpp>

#include "ServerConfig.hpp"
#include "TlsConfig.hpp"
#include "ValidationConfig.hpp"

namespace app
{

PROTOCOL_JSON_SEREALIZE(utils::StringRule);
PROTOCOL_JSON_SEREALIZE(app::TlsConfig);
PROTOCOL_JSON_SEREALIZE(app::ValidationConfig);
PROTOCOL_JSON_SEREALIZE(app::ServerConfig);

} // namespace app
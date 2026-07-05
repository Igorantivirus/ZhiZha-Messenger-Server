#pragma once

#include <string>

namespace auth
{

struct TokenPair
{
    std::string access;
    std::string refresh;
};

} // namespace auth
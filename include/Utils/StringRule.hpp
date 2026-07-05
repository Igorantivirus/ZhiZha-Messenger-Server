#pragma once

#include <nlohmann/json.hpp>

#include <Protocol/Common/Parsing.hpp>

namespace utils
{

struct StringRule
{
public:
    std::size_t minLength = 1;
    std::size_t maxLength = 100;

    bool allowLetters = true;
    bool allowDigits = true;
    bool mustStartWithLetter = true;
    std::string allowedSpecialSymbols = {};

public:
    bool check(const std::string &value) const;

private:
    bool isAllowed(char c) const;
};

PROTOCOL_JSON_SEREALIZE(StringRule)

} // namespace utils
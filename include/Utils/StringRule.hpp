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

    // Если true — проверяется только длина, все правила по символам
    // (allowLetters/allowDigits/mustStartWithLetter/allowedSpecialSymbols)
    // игнорируются. Для полей вроде displayName, где допустим почти любой текст.
    bool onlyLengthCheck = false;

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
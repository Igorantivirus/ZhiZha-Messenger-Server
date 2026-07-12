#include <Utils/StringRule.hpp>

constexpr bool isLetter(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
constexpr bool isDigit(const char c)
{
    return c >= '0' && c <= '9';
}

namespace utils
{

bool StringRule::check(const std::string &value) const
{
    if (value.size() < minLength || value.size() > maxLength)
        return false;
    if (value.empty())
        return minLength == 0;
    // Длина уже проверена — символьные правила пропускаем.
    if (onlyLengthCheck)
        return true;
    if (mustStartWithLetter && !isLetter(value.front()))
        return false;
    for (const char c : value)
        if (!isAllowed(c))
            return false;
    return true;
}

bool StringRule::isAllowed(char c) const
{
    if (allowLetters && isLetter(c))
        return true;
    if (allowDigits && isDigit(c))
        return true;
    return allowedSpecialSymbols.find(c) != std::string::npos;
}

} // namespace utils
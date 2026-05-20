#pragma once

#include <cstddef>
#include <limits>
#include <string>

#include <nlohmann/json.hpp>

namespace validation
{

// Одно декларативное правило для строкового поля: длина + допустимый набор
// символов. Конфигурируется данными (грузится из конфига), а не кодом, —
// поэтому никаких магических чисел в самих валидаторах.
//
// Это переиспользуемый кирпич: username, password, displayName, roomName —
// каждый просто свой экземпляр StringRule с разными параметрами.
struct StringRule
{
    std::size_t minLength = 0;
    std::size_t maxLength = std::numeric_limits<std::size_t>::max();

    bool allowLetters = true;        // a-z A-Z
    bool allowDigits = true;         // 0-9
    bool mustStartWithLetter = false;// первый символ — буква
    std::string allowedSpecialSymbols; // прочие разрешённые символы

    bool check(const std::string& value) const
    {
        if (value.size() < minLength || value.size() > maxLength)
            return false;
        if (value.empty())
            return minLength == 0;
        if (mustStartWithLetter && !isLetter(value.front()))
            return false;
        for (const char c : value)
            if (!isAllowed(c))
                return false;
        return true;
    }

private:
    static bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    static bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }
    bool isAllowed(char c) const
    {
        if (allowLetters && isLetter(c))
            return true;
        if (allowDigits && isDigit(c))
            return true;
        return allowedSpecialSymbols.find(c) != std::string::npos;
    }
};

// Поля, отсутствующие в JSON, берут значения из StringRule по умолчанию.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    StringRule, minLength, maxLength, allowLetters, allowDigits, mustStartWithLetter, allowedSpecialSymbols)

} // namespace validation

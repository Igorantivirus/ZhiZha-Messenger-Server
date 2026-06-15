#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace utils
{
template <typename T>
std::optional<T> parseQuery(const char *value)
{
    if (!value)
        return std::nullopt;
    std::string_view sv{value};

    if constexpr (std::is_same_v<T, std::string>)
        return std::optional<T>{std::string(sv)};
    else if constexpr (std::is_same_v<T, std::string_view>)
        return std::optional<T>{sv};
    else
    {
        T result{};
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
        if (ec != std::errc{} || ptr != sv.data() + sv.size())
            return std::nullopt;
        return std::optional<T>{result};
    }
}
template <typename T>
T parseQuery(const char *value, const T defaultValue)
{
    std::optional<T> res = parseQuery<T>(value);
    return res.has_value() ? res.value() : defaultValue;
}
} // namespace utils
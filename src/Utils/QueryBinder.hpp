#pragma once

#include <optional>
#include <string>
#include <type_traits>

#include <boost/pfr.hpp>
#include <crow/crow.h>

namespace utils
{

namespace detail
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

// Распознаём std::optional<T>
template <typename T>
struct IsOptional : std::false_type
{
};
template <typename U>
struct IsOptional<std::optional<U>> : std::true_type
{
};

template <typename T>
constexpr bool isOptionalV = IsOptional<T>::value;

} // namespace detail

// Парсит QueryParams из crow::request, используя имена полей структуры
// как имена query-параметров.
//
// Поля std::optional<T>     -> опциональные, отсутствие = nullopt
// Поля T (без optional)     -> обязательные, отсутствие = ошибка
//
// Возвращает nullopt при ошибке валидации.
template <typename QueryParams>
std::optional<QueryParams> parseQueryParams(const crow::request &req)
{
    QueryParams result{};

    // Пустая структура — успех без работы.
    if constexpr (boost::pfr::tuple_size_v<QueryParams> == 0)
        return result;
    else
    {
        bool ok = true;
        constexpr auto names = boost::pfr::names_as_array<QueryParams>();

        boost::pfr::for_each_field(result, [&](auto &field, auto idx)
        {
            if (!ok)
                return;

            using FieldType = std::remove_reference_t<decltype(field)>;
            const std::string name{names[idx]};
            const char *raw = req.url_params.get(name.c_str());

            if constexpr (detail::isOptionalV<FieldType>)
            {
                using Inner = typename FieldType::value_type;
                if (raw)
                {
                    auto parsed = parseQuery<Inner>(raw);
                    if (!parsed)
                    {
                        ok = false;
                        return;
                    }
                    field = *parsed;
                }
                // raw == nullptr -> оставляем nullopt
            }
            else
            {
                if (!raw)
                {
                    ok = false;
                    return;
                }
                auto parsed = parseQuery<FieldType>(raw);
                if (!parsed)
                {
                    ok = false;
                    return;
                }
                field = *parsed;
            }
        });

        if (!ok)
            return std::nullopt;
        return result;
    }
}

} // namespace utils
#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <type_traits>

#include <boost/pfr.hpp>
#include <crow/crow.h>

namespace utils
{

class QueryParser
{
private:
    template <typename T>
    struct IsOptional : std::false_type
    {
    };
    template <typename U>
    struct IsOptional<std::optional<U>> : std::true_type
    {
    };
    template <typename T>
    static constexpr bool isOptionalV = IsOptional<T>::value;

public:
    template <typename T>
    static std::optional<T> parse(const std::string_view value)
    {
        if (value.empty())
            return std::nullopt;

        if constexpr (std::is_same_v<T, std::string>)
            return std::optional<T>{std::string(value)};
        else if constexpr (std::is_same_v<T, std::string_view>)
            return std::optional<T>{value};
        else if constexpr (std::is_same_v<T, bool>)
        {
            // from_chars не парсит bool — обрабатываем вручную
            if (value == "1" || value == "true")
                return std::optional<T>{true};
            if (value == "0" || value == "false")
                return std::optional<T>{false};
            return std::nullopt;
        }
        else if constexpr ((std::is_integral_v<T> && !std::is_same_v<T, char> && !std::is_same_v<T, wchar_t>) || std::is_floating_point_v<T>)
        {
            T result{};
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
            if (ec != std::errc{} || ptr != value.data() + value.size())
                return std::nullopt;
            return std::optional<T>{result};
        }
        else
        {
            static_assert(sizeof(T) == 0, "QueryParser::parse: unsupported type T");
        }
    }

    template <typename T>
    static T parse(const std::string_view value, const T defaultValue)
    {
        std::optional<T> res = parse<T>(value); // было parseQuery<T> — такой функции нет
        return res.has_value() ? res.value() : defaultValue;
    }

    template <typename QueryParams>
    static std::optional<QueryParams> parseParams(const crow::request &req)
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
                // get принимает const std::string&, а names[idx] — string_view
                const char *raw = req.url_params.get(std::string(names[idx]));

                if constexpr (isOptionalV<FieldType>)
                {
                    using Inner = typename FieldType::value_type;
                    if (raw)
                    {
                        auto parsed = parse<Inner>(raw);
                        if (!parsed)
                        {
                            ok = false;
                            return;
                        }
                        field = parsed.value();
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
                    auto parsed = parse<FieldType>(raw);
                    if (!parsed)
                    {
                        ok = false;
                        return;
                    }
                    field = parsed.value();
                }
            });

            if (!ok)
                return std::nullopt;
            return result;
        }
    }
};

} // namespace utils
#pragma once

#ifdef PROTOCOL_USE_JSON_PARSING

#include <boost/pfr.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

using json = nlohmann::json;

namespace protocol::detail
{

// Базовый шаблон (не определен, чтобы вызывать ошибку компиляции при передаче неподдерживаемых типов)
template <typename T, typename Enable = void>
struct json_io_impl;

// Специализация для перечислений (enums)
template <typename T>
struct json_io_impl<T, std::enable_if_t<std::is_enum_v<T>>>
{
    static void to_json(nlohmann::json &j, const T &v, const char *type_name)
    {
        auto name = magic_enum::enum_name(v);
        if (name.empty())
        {
            throw nlohmann::json::other_error::create(
                501,
                "Cannot serialize " + std::string(type_name) + ": value " +
                    std::to_string(static_cast<std::underlying_type_t<T>>(v)) +
                    " is not a named enumerator",
                nullptr);
        }
        j = std::string(name);
    }
    static void from_json(const nlohmann::json &j, T &v, const char *type_name)
    {
        if (!j.is_string())
        {
            throw nlohmann::json::type_error::create(
                302,
                "Cannot deserialize " + std::string(type_name) + ": expected string, got " +
                    std::string(j.type_name()),
                &j);
        }
        const auto &s = j.get_ref<const std::string &>();
        auto value = magic_enum::enum_cast<T>(s);
        if (!value.has_value())
        {
            throw nlohmann::json::other_error::create(
                501,
                "Cannot deserialize " + std::string(type_name) + ": '" + s +
                    "' is not a valid enumerator",
                &j);
        }
        v = *value;
    }
};

// Специализация для структур и классов (исключая enums)
template <typename T>
struct json_io_impl<T, std::enable_if_t<!std::is_enum_v<T> && std::is_class_v<T>>>
{
    static void to_json(nlohmann::json &j, const T &v, const char * /*type_name*/)
    {
        constexpr auto names = boost::pfr::names_as_array<T>();
        boost::pfr::for_each_field(v, [&](const auto &field, std::size_t idx)
        {
            j[std::string(names[idx])] = field;
        });
    }
    static void from_json(const nlohmann::json &j, T &v, const char * /*type_name*/)
    {
        constexpr auto names = boost::pfr::names_as_array<T>();
        boost::pfr::for_each_field(v, [&](auto &field, std::size_t idx)
        {
            std::string name{names[idx]};
            if (j.contains(name))
            {
                j.at(name).get_to(field);
            }
        });
    }
};

} // namespace protocol::detail

// Единый макрос для генерации to_json и from_json
#define PROTOCOL_JSON_SEREALIZE(Type)                                 \
    inline void to_json(nlohmann::json &j, const Type &v)             \
    {                                                                 \
        protocol::detail::json_io_impl<Type>::to_json(j, v, #Type);   \
    }                                                                 \
    inline void from_json(const nlohmann::json &j, Type &v)           \
    {                                                                 \
        protocol::detail::json_io_impl<Type>::from_json(j, v, #Type); \
    }
#else // !PROTOCOL_USE_JSON_PARSING

#define PROTOCOL_JSON_SEREALIZE(Type)

#endif // PROTOCOL_USE_JSON_PARSING
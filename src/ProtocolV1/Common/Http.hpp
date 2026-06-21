#pragma once

#include <string_view>
#include <type_traits>

#include <boost/beast/http/verb.hpp>

#ifndef API
#define API "/api/v1/"
#endif

namespace protocol
{

using Path = std::string_view;
using HttpMethod = boost::beast::http::verb;

// static constexpr неявно const, поэтому снимаем cv перед сравнением типов —
// иначе decltype(T::path) == const Path и is_same_v возвращает false.
template <typename T>
concept Operation = requires {
    typename T::Request;
    typename T::Response;
    typename T::PathParams;
    typename T::QueryParams;
    requires std::is_same_v<std::remove_cv_t<decltype(T::path)>, Path>;
    requires std::is_same_v<std::remove_cv_t<decltype(T::method)>, HttpMethod>;
};

template <typename Op>
constexpr bool hasRequestBody = !std::is_void_v<typename Op::Request>;

template <typename Op>
constexpr bool hasResponseBody = !std::is_void_v<typename Op::Response>;

} // namespace protocol
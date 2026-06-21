// ===== RouteBinder.hpp =====
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/pfr.hpp>
#include <crow/crow.h>

#include <ProtocolV1/Common/Http.hpp>

namespace utils
{

namespace detail
{

constexpr crow::HTTPMethod toCrowMethod(protocol::HttpMethod m)
{
    using P = protocol::HttpMethod;
    using C = crow::HTTPMethod;
    switch (m)
    {
    case P::get:
        return C::Get;
    case P::post:
        return C::Post;
    case P::put:
        return C::Put;
    case P::patch:
        return C::Patch;
    case P::delete_:
        return C::Delete;
    default:
        return C::Get;
    }
}

template <typename PathParams, typename... Args>
PathParams makePathParams(Args... args)
{
    static_assert(sizeof...(Args) == boost::pfr::tuple_size_v<PathParams>,
                  "Path parameter count mismatch");

    PathParams p{};
    if constexpr (sizeof...(Args) > 0)
    {
        auto tup = std::make_tuple(args...);
        boost::pfr::for_each_field(p, [&](auto &field, auto idx)
        {
            using F = std::remove_reference_t<decltype(field)>;
            field = static_cast<F>(std::get<idx>(tup));
        });
    }
    return p;
}

template <std::size_t N, typename Wrap>
void registerRoute(crow::SimpleApp &app, std::string_view path, crow::HTTPMethod method, Wrap wrap)
{
    auto &route = app.route_dynamic(std::string(path));
    if constexpr (N == 0)
        route.methods(method)([wrap = std::move(wrap)](const crow::request &req)
        {
            return wrap(req);
        });
    else if constexpr (N == 1)
        route.methods(method)([wrap = std::move(wrap)](const crow::request &req, std::uint64_t a)
        {
            return wrap(req, a);
        });
    else if constexpr (N == 2)
        route.methods(method)([wrap = std::move(wrap)](const crow::request &req, std::uint64_t a, std::uint64_t b)
        {
            return wrap(req, a, b);
        });
    else if constexpr (N == 3)
        route.methods(method)([wrap = std::move(wrap)](const crow::request &req, std::uint64_t a, std::uint64_t b, std::uint64_t c)
        {
            return wrap(req, a, b, c);
        });
    else
        static_assert(N <= 3, "Too many path parameters");
}

} // namespace detail

// =====================================================================
// Облегченный базовый биндер. Принимает только лямбду-обработчик.
// =====================================================================
template <typename Op, typename Handler>
void bindHandler(crow::SimpleApp &app, Handler handler)
{
    constexpr auto N = boost::pfr::tuple_size_v<typename Op::PathParams>;

    auto wrap = [handler = std::move(handler)](const crow::request &req, auto... pathArgs) -> crow::response
    {
        typename Op::PathParams path{};
        if constexpr (N > 0)
        {
            try
            {
                path = detail::makePathParams<typename Op::PathParams>(pathArgs...);
            }
            catch (...)
            {
                return crow::response(400, "Bad Request: Invalid path parameters");
            }
        }

        auto query = parseQueryParams<typename Op::QueryParams>(req);
        if (!query)
        {
            return crow::response(400, "Bad Request: Invalid or missing query parameters");
        }

        return std::invoke(handler, req, path, *query);
    };

    detail::registerRoute<N>(app, Op::path, detail::toCrowMethod(Op::method), std::move(wrap));
}

} // namespace helpers
#pragma once

#include <utility>

namespace utils
{
template <class Class, typename Result, typename... Args>
constexpr auto bindMethod(Class *self, Result (Class::*method)(Args...)) noexcept
{
    return [self, method](Args &&...args) noexcept(noexcept((self->*method)(std::forward<Args>(args)...)))
               -> decltype(auto)
    {
        return (self->*method)(std::forward<Args>(args)...);
    };
}
template <class Class, typename Result, typename... Args>
constexpr auto bindMethod(const Class *self, Result (Class::*method)(Args...) const) noexcept
{
    return [self, method](Args &&...args) noexcept(noexcept((self->*method)(std::forward<Args>(args)...)))
               -> decltype(auto)
    {
        return (self->*method)(std::forward<Args>(args)...);
    };
}
} // namespace core
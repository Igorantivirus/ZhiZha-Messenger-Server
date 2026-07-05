#pragma once

#include <ctime>
#include <string>

#include <Utils/Types.hpp>

namespace auth
{

// Полная публичная карточка пользователя для экрана профиля.
// Намеренно НЕ содержит passwordHash — это контракт безопасности на уровне типа.
struct UserFullView
{
    utils::UserId id;
    std::string username;
    std::string displayName;
    utils::Country country;
    std::time_t birthDate;
    std::time_t registerTime;
};

} // namespace auth

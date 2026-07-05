#pragma once

#include <string>

#include <Utils/Types.hpp>

namespace auth
{

// Минимальная публичная карточка пользователя: id + displayName.
// Используется там, где нужно «подписать» сообщение/участника именем —
// без выдачи приватных полей профиля.
struct UserPublicView
{
    utils::UserId id;
    std::string displayName;
};

} // namespace auth

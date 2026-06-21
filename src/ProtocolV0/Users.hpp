#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <unordered_map>

#include "Types.hpp"

// вспомогательные структуры
namespace protocol::users
{
// Страна пользователя. Сериализуется строкой (magic_enum).
enum class Country : std::uint8_t
{
    None, // не указана
    Ru,   // Россия
    By,   // Беларусь
    Kz,   // Казахстан
    Ua,   // Украина
    Us,   // США
    Gb,   // Великобритания
    De,   // Германия
    Fr,   // Франция
    Cn,   // Китай
    Jp,   // Япония
    In,   // Индия
    Br    // Бразилия
};

struct UserEditableInfo
{
    std::string username;
    std::string displayname;
    std::time_t birthDate;
    Country country;
};

// Публичная информация о пользователе. Вся информация о юзере публичная,
// поэтому здесь же несём дату рождения, страну и время регистрации.
struct UserDisplayInfo
{
    UserEditableInfo info;
    std::time_t registerTime; // время первой регистрации (ставит сервер)
};
} // namespace protocol::users

// клиент -> сервер
namespace protocol::users
{
struct ChangeUserRequest
{
    UserEditableInfo newInfo;
};

} // namespace protocol::users

// сервер -> клиент
namespace protocol::users
{
struct MeResponse
{
    UserId userId;
    UserDisplayInfo display;
};
struct UserResponse
{
    UserId userId;
    UserDisplayInfo display;
};

struct UsersLoopByExampleResponse
{
    std::unordered_map<UserId, UserDisplayInfo> users;
};
} // namespace protocol::users
#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <unordered_map>

#include "Types.hpp"
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

// Публичная информация о пользователе. Вся информация о юзере публичная,
// поэтому здесь же несём дату рождения, страну и время регистрации.
struct UserDisplayInfo
{
    std::string username;
    std::string displayname;
    std::time_t birthDate;    // дата рождения (присылается клиентом, не валидируется)
    Country country;          // страна пользователя
    std::time_t registerTime; // время первой регистрации (ставит сервер)
};
} // namespace protocol::users

// клиент -> сервер
namespace protocol::users
{
} // namespace protocol::users

// сервер -> клиент
namespace protocol::users
{
struct MeResponse
{
    UserId userId;
    std::string username;
    std::string displayname;
    std::time_t registerTime;
    std::time_t birthDate;
    Country country;
};
struct UserResponse
{
    UserId userId;
    std::string username;
    std::string displayname;
    std::time_t registerTime;
    std::time_t birthDate;
    Country country;
};

struct UsersLoopByExampleResponse
{
    std::unordered_map<UserId, UserDisplayInfo> users;
};
} // namespace protocol::users
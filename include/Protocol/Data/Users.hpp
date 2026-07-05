#pragma once

#include <ctime>
#include <string>

#include <Protocol/Common/Parsing.hpp>
#include <Protocol/Common/Types.hpp>
#include <Protocol/Data/Users.hpp>

namespace protocol::data
{

// Главная отображаемая информация о пользователе
struct UserDisplayInfo
{
    std::string displayName;
};
PROTOCOL_JSON_SEREALIZE(UserDisplayInfo)

// Дополнительная открытая информация о пользователе
struct UserAdditionalInfo
{
    std::string username;
    std::time_t birthDate;
    Country country;
    std::time_t registerTime;
};
PROTOCOL_JSON_SEREALIZE(UserAdditionalInfo)

// Полная информация о пользователе
struct UserFullInfo
{
    UserId userId;
    UserDisplayInfo displayInfo;
    UserAdditionalInfo additionalInfo;
};
PROTOCOL_JSON_SEREALIZE(UserFullInfo)

// То, что пользователь может поменять сам
struct UserEditableInfo
{
    std::string username;
    std::string displayname;
    std::time_t birthDate;
    Country country;
};
PROTOCOL_JSON_SEREALIZE(UserEditableInfo)

} // namespace protocol::data
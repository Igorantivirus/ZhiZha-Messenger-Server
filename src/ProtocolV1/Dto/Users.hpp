#pragma once

#include <unordered_map>

#include <ProtocolV1/Common/Parsing.hpp>
#include <ProtocolV1/Common/Types.hpp>
#include <ProtocolV1/Data/Users.hpp>

namespace protocol::dto
{

// Получение пользователей по запросу
struct LoopUsersResponseDto
{
    std::unordered_map<UserId, data::UserDisplayInfo> users;
};
PROTOCOL_JSON_SEREALIZE(LoopUsersResponseDto)

// Запрос на изменение настроек
struct ChangeMeRequestDto
{
    data::UserEditableInfo info;
};
PROTOCOL_JSON_SEREALIZE(ChangeMeRequestDto)

// Запрос получения информации о пользователе
struct UserResponseDto
{
    data::UserFullInfo info;
};
PROTOCOL_JSON_SEREALIZE(UserResponseDto)

} // namespace protocol::dto
#pragma once

#include <ctime>
#include <string>

#include <Utils/Types.hpp>

namespace chat
{

// Доменное представление комнаты. participantsCount не хранится — считается
// на лету через MembersRepository::findCountMembersById, попадает в RoomForUser.
struct Room
{
    utils::RoomId id;
    std::string name;
    utils::RoomInfo info;
    std::time_t createdAt;
};

} // namespace chat

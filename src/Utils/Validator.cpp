#include <Utils/Validator.hpp>

namespace utils
{

Validator::Validator(
    StringRule username,
    StringRule password,
    StringRule displayName,
    StringRule roomName)
    : username_(username),
      password_(password),
      displayName_(displayName),
      roomName_(roomName)
{
}

bool Validator::isValidUsername(const std::string &username) const
{
    return username_.check(username);
}
bool Validator::isValidPassword(const std::string &password) const
{
    return password_.check(password);
}
bool Validator::isValidDisplayName(const std::string &displayName) const
{
    return displayName_.check(displayName);
}
bool Validator::isValidRoomName(const std::string &roomName) const
{
    return roomName_.check(roomName);
}

} // namespace utils
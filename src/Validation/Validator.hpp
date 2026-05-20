#pragma once

#include <string>

#include <Auth/Interfaces/ICredentialsValidator.hpp>
#include <Validation/StringRule.hpp>

namespace validation
{

// Централизованный валидатор: одно место, которое знает правила для всех
// именованных полей системы. Внутри — композиция StringRule (по одному на
// поле), без иерархии наследников: добавить новое поле = добавить ещё один
// StringRule, а не новый класс.
//
// Реализует ICredentialsValidator (нужен AuthService), а методы для чата
// (displayName/roomName) добавлены сверху — ими воспользуется ChatService.
class Validator : public ICredentialsValidator
{
public:
    Validator(StringRule username, StringRule password, StringRule displayName, StringRule roomName)
        : username_(std::move(username)),
          password_(std::move(password)),
          displayName_(std::move(displayName)),
          roomName_(std::move(roomName))
    {
    }

    bool isValidUsername(const std::string &username) const override
    {
        return username_.check(username);
    }

    bool isValidPassword(const std::string &password) const override
    {
        return password_.check(password);
    }

    bool isValidDisplayName(const std::string &displayName) const
    {
        return displayName_.check(displayName);
    }

    bool isValidRoomName(const std::string &roomName) const
    {
        return roomName_.check(roomName);
    }

private:
    StringRule username_;
    StringRule password_;
    StringRule displayName_;
    StringRule roomName_;
};

} // namespace validation

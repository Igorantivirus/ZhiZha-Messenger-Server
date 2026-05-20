#pragma once

#include <string>

class ICredentialsValidator
{
public:
    virtual ~ICredentialsValidator() = default;
    virtual bool isValidUsername(const std::string &username) const = 0;
    virtual bool isValidPassword(const std::string &password) const = 0;
};
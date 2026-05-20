#pragma once

#include <Auth/Interfaces/ICredentialsValidator.hpp>

class CredentialsValidator : public ICredentialsValidator
{
public:
    static constexpr const char specialSymbolsOfName[] = "-_";
    static constexpr const char specialSymbolsOfPassword[] = "-_()@#$%^&*-!?=+/";

    static constexpr const std::pair<std::size_t, std::size_t> usernameSizeLimits = {4, 32};
    static constexpr const std::pair<std::size_t, std::size_t> passwordSizeLimits = {4, 32};

public:
    CredentialsValidator() = default;

    bool isValidUsername(const std::string &username) const override
    {
        if (username.size() < usernameSizeLimits.first || username.size() > usernameSizeLimits.second)
            return false;
        if (!isLetter(username[0]))
            return false;
        for (const char c : username)
            if (!isAcceptableNameSymbol(c))
                return false;
        return true;
    }

    bool isValidPassword(const std::string &password) const override
    {
        if (password.size() < passwordSizeLimits.first || password.size() > passwordSizeLimits.second)
            return false;
        for (const char c : password)
            if (!isAcceptablePasswordSymbol(c))
                return false;
        return true;
    }

private:
    bool isLetter(const char c) const
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    bool isNumber(const char c) const
    {
        return (c >= '0' && c <= '9');
    }
    bool isAcceptableNameSymbol(const char c) const
    {
        if (isLetter(c) || isNumber(c))
            return true;
        for (const char s : specialSymbolsOfName)
            if (c == s)
                return true;
        return false;
    }
    bool isAcceptablePasswordSymbol(const char c) const
    {
        if (isLetter(c) || isNumber(c))
            return true;
        for (const char s : specialSymbolsOfPassword)
            if (c == s)
                return true;
        return false;
    }
};
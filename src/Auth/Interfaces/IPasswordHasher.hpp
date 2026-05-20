#pragma once

#include <string>

class IPasswordHasher
{
public:
    virtual ~IPasswordHasher() = default;
    virtual std::string hash(const std::string &password) = 0;
    virtual bool verify(const std::string &password, const std::string &stored_hash) = 0;
};
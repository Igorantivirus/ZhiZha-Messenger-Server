#pragma once

#include <cstdint>

namespace auth
{

enum class AuthError : std::uint8_t
{
    UsernameTaken,
    InvalidCredentials,
    WeakPassword,
    UsernameValidation,
    InvalidToken,
    TokenExpired,
    TokenReused,
    NoSendAccess,
    UserNotFound
};

}
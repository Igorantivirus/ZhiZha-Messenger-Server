#pragma once

#include <cstdint>

enum class AuthError : std::uint8_t
{
    UsernameTaken,
    InvalidCredentials,
    WeakPassword,
    UsernameValidation,
    InvalidToken,
    TokenExpired,
    TokenReused
};
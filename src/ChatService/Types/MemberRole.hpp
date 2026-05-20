#pragma once

#include <cstdint>

enum class MemberRole : std::uint8_t
{
    Owner,
    Admin,
    Member
};
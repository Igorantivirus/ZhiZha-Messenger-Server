#pragma once

#include "Message.hpp"
#include "Room.hpp"

struct RoomWithLastMessage
{
    Room room;
    Message msg;
};
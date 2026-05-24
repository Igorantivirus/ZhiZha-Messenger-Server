#pragma once

#include <vector>

#include "Message.hpp"

struct MessagePage
{
    std::vector<Message> messages; // в хронологическом порядке (ASC)
    bool hasMore;                  // есть ли ещё раньше первого сообщения здесь
};
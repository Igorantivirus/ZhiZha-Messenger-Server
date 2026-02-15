#pragma once

#include <string>

#include "protocol/JsonMessages.hpp"

class JsonPacker
{
public:
    [[nodiscard]] static std::string packServerHello(const ServerHelloPayload& payload);
    [[nodiscard]] static std::string packRegistration(const ServerRegistrationPayload& payload);
    [[nodiscard]] static std::string packError(const ServerErrorPayload& payload);
    [[nodiscard]] static std::string packChatMessage(const ServerChatMessagePayload& payload);
    [[nodiscard]] static std::string packRoomCreated(const ServerRoomCreatedPayload& payload);
    [[nodiscard]] static std::string packRoomLeft(const ServerRoomLeftPayload& payload);
    [[nodiscard]] static std::string packServerInfo(bool alive, const std::string& serverName);
};


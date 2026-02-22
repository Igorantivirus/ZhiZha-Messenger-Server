#pragma once

#include <string>

#include "protocol/JsonMessages.hpp"

class JsonPacker
{
public:
    // Client -> Server
    [[nodiscard]] static std::string packRegisterRequest(const ClientRegisterRequest& payload);
    [[nodiscard]] static std::string packChatMessageRequest(const ClientChatMessageRequest& payload);
    [[nodiscard]] static std::string packDataRequest(const ClientDataRequest& payload);
    [[nodiscard]] static std::string packCreateRoomRequest(const ClientCreateRoomRequest& payload);
    [[nodiscard]] static std::string packLeaveRoomRequest(const ClientLeaveRoomRequest& payload);

    // Server -> Client
    [[nodiscard]] static std::string packServerHello(const ServerHelloPayload& payload);
    [[nodiscard]] static std::string packRegistration(const ServerRegistrationPayload& payload);
    [[nodiscard]] static std::string packError(const ServerErrorPayload& payload);
    [[nodiscard]] static std::string packChatMessage(const ServerChatMessagePayload& payload);
    [[nodiscard]] static std::string packRoomCreated(const ServerRoomCreatedPayload& payload);
    [[nodiscard]] static std::string packRoomLeft(const ServerRoomLeftPayload& payload);
    [[nodiscard]] static std::string packRequestChatsPayload(const ServerChatsRequestPayload& payload);
    [[nodiscard]] static std::string packRequestUsersPayload(const ServerUsersRequestPayload& payload);

    // HTTP responses
    [[nodiscard]] static std::string packServerInfo(bool alive, const std::string& serverName);
};

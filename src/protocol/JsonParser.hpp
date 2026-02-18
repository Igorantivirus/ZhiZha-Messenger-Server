#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "protocol/JsonMessages.hpp"

template<typename T>
inline std::optional<T> getJsonField(const nlohmann::json& payload, const std::string& fieldName)
{
    const auto fieldIt = payload.find(fieldName);
    if (fieldIt == payload.end() || fieldIt->is_null())
    {
        return std::nullopt;
    }

    try
    {
        return fieldIt->get<T>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

class JsonParser
{
public:
    [[nodiscard]] static std::optional<nlohmann::json> parseJson(const std::string& rawPayload);
    [[nodiscard]] static std::optional<nlohmann::json> parseJson(const std::string_view rawPayload);
    [[nodiscard]] static std::optional<std::string> parseMessageType(const nlohmann::json& payload);

    // Client -> Server
    [[nodiscard]] static std::optional<ClientRegisterRequest> parseRegisterRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientChatMessageRequest> parseChatMessageRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientDataRequest> parseDataRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientCreateRoomRequest> parseCreateRoomRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientLeaveRoomRequest> parseLeaveRoomRequest(const nlohmann::json& payload);

    // Server -> Client
    [[nodiscard]] static std::optional<ServerHelloPayload> parseServerHelloPayload(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ServerRegistrationPayload> parseServerRegistrationPayload(
        const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ServerErrorPayload> parseServerErrorPayload(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ServerChatMessagePayload> parseServerChatMessagePayload(
        const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ServerRoomCreatedPayload> parseServerRoomCreatedPayload(
        const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ServerRoomLeftPayload> parseServerRoomLeftPayload(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ServerChatsRequestPayload> parseServerChatsRequestPayload(const nlohmann::json& payload);

    // HTTP responses
    [[nodiscard]] static std::optional<bool> parseServerAlive(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<std::string> parseServerName(const nlohmann::json& payload);
};

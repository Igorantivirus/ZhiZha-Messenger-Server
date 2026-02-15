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
    [[nodiscard]] static std::optional<std::string> parseMessageType(const nlohmann::json& payload);

    [[nodiscard]] static std::optional<ClientRegisterRequest> parseRegisterRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientChatMessageRequest> parseChatMessageRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientCreateRoomRequest> parseCreateRoomRequest(const nlohmann::json& payload);
    [[nodiscard]] static std::optional<ClientLeaveRoomRequest> parseLeaveRoomRequest(const nlohmann::json& payload);
};


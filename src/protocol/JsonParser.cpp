#include "protocol/JsonParser.hpp"

std::optional<nlohmann::json> JsonParser::parseJson(const std::string& rawPayload)
{
    auto payload = nlohmann::json::parse(rawPayload, nullptr, false);
    if (payload.is_discarded() || !payload.is_object())
    {
        return std::nullopt;
    }
    return payload;
}

std::optional<std::string> JsonParser::parseMessageType(const nlohmann::json& payload)
{
    return getJsonField<std::string>(payload, "type");
}

std::optional<ClientRegisterRequest> JsonParser::parseRegisterRequest(const nlohmann::json& payload)
{
    const auto publicKey = getJsonField<std::string>(payload, "public-key");
    if (!publicKey.has_value())
    {
        return std::nullopt;
    }

    ClientRegisterRequest request;
    request.publicKey = *publicKey;
    request.username = getJsonField<std::string>(payload, "username").value_or("");
    request.clientVersion = getJsonField<std::string>(payload, "client-version").value_or("");
    return request;
}

std::optional<ClientChatMessageRequest> JsonParser::parseChatMessageRequest(const nlohmann::json& payload)
{
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto chatId = getJsonField<IDType>(payload, "chat-id");
    const auto message = getJsonField<std::string>(payload, "message");
    if (!userId.has_value() || !chatId.has_value() || !message.has_value())
    {
        return std::nullopt;
    }

    ClientChatMessageRequest request;
    request.userId = *userId;
    request.chatId = *chatId;
    request.message = *message;
    request.clientMessageId = getJsonField<std::uint64_t>(payload, "client-message-id").value_or(0);
    return request;
}

std::optional<ClientCreateRoomRequest> JsonParser::parseCreateRoomRequest(const nlohmann::json& payload)
{
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto participantUserIds = getJsonField<std::vector<IDType>>(payload, "participant-user-ids");
    if (!userId.has_value() || !participantUserIds.has_value())
    {
        return std::nullopt;
    }

    ClientCreateRoomRequest request;
    request.userId = *userId;
    request.participantUserIds = *participantUserIds;
    request.isPrivate = getJsonField<bool>(payload, "is-private").value_or(true);
    return request;
}

std::optional<ClientLeaveRoomRequest> JsonParser::parseLeaveRoomRequest(const nlohmann::json& payload)
{
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto chatId = getJsonField<IDType>(payload, "chat-id");
    if (!userId.has_value() || !chatId.has_value())
    {
        return std::nullopt;
    }

    ClientLeaveRoomRequest request;
    request.userId = *userId;
    request.chatId = *chatId;
    return request;
}


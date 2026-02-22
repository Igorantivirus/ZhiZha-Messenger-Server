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
std::optional<nlohmann::json> JsonParser::parseJson(const std::string_view rawPayload)
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
    request.password = getJsonField<std::string>(payload, "password").value_or("");
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

std::optional<ClientDataRequest> JsonParser::parseDataRequest(const nlohmann::json& payload)
{
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto dataType = getJsonField<std::string>(payload, "data-type").value_or(
        getJsonField<std::string>(payload, "dataType").value_or(""));
    if (!userId.has_value() || dataType.empty())
    {
        return std::nullopt;
    }

    ClientDataRequest request;
    request.userId = *userId;
    request.dataType = dataType;
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

std::optional<ServerHelloPayload> JsonParser::parseServerHelloPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    const auto authorized = getJsonField<bool>(payload, "authorized");
    const auto timeoutSeconds = getJsonField<std::uint32_t>(payload, "registration-timeout-seconds");
    const auto serverName = getJsonField<std::string>(payload, "server-name");
    if (!type.has_value() || *type != "hello" || !authorized.has_value() || !timeoutSeconds.has_value() ||
        !serverName.has_value())
    {
        return std::nullopt;
    }

    ServerHelloPayload result{};
    result.authorized = *authorized;
    result.registrationTimeoutSeconds = *timeoutSeconds;
    result.serverName = *serverName;
    return result;
}

std::optional<ServerRegistrationPayload> JsonParser::parseServerRegistrationPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    const auto registered = getJsonField<bool>(payload, "registered");
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto serverPublicKey = getJsonField<std::string>(payload, "server-public-key");
    const auto serverName = getJsonField<std::string>(payload, "server-name");
    if (!type.has_value() || *type != "register-result" || !registered.has_value() || !userId.has_value() ||
        !serverPublicKey.has_value() || !serverName.has_value())
    {
        return std::nullopt;
    }

    ServerRegistrationPayload result{};
    result.type = *type;
    result.registered = *registered;
    result.userId = *userId;
    result.serverPublicKey = *serverPublicKey;
    result.serverName = *serverName;
    result.protocolVersion = getJsonField<std::string>(payload, "protocol-version").value_or("1.0");
    return result;
}

std::optional<ServerErrorPayload> JsonParser::parseServerErrorPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    const auto code = getJsonField<std::string>(payload, "code");
    const auto message = getJsonField<std::string>(payload, "message");
    if (!type.has_value() || !code.has_value() || !message.has_value())
    {
        return std::nullopt;
    }

    ServerErrorPayload result{};
    result.code = *code;
    result.message = *message;
    return result;
}

std::optional<ServerChatMessagePayload> JsonParser::parseServerChatMessagePayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto userName = getJsonField<std::string>(payload, "username");
    const auto chatId = getJsonField<IDType>(payload, "chat-id");
    const auto message = getJsonField<std::string>(payload, "message");
    const auto serverMessageId = getJsonField<std::uint64_t>(payload, "server-message-id");
    if (!type.has_value() || *type != "chat-msg" || !userId.has_value() || !chatId.has_value() || !message.has_value() ||
        !serverMessageId.has_value() || !userName.has_value())
    {
        return std::nullopt;
    }

    ServerChatMessagePayload result{};
    result.userId = *userId;
    result.userName = *userName;
    result.chatId = *chatId;
    result.message = *message;
    result.serverMessageId = *serverMessageId;
    return result;
}

std::optional<ServerRoomCreatedPayload> JsonParser::parseServerRoomCreatedPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    const auto created = getJsonField<bool>(payload, "created");
    const auto chatId = getJsonField<IDType>(payload, "chat-id");
    const auto participantUserIds = getJsonField<std::vector<IDType>>(payload, "participant-user-ids");
    if (!type.has_value() || *type != "room-created" || !created.has_value() || !chatId.has_value() ||
        !participantUserIds.has_value())
    {
        return std::nullopt;
    }

    ServerRoomCreatedPayload result{};
    result.created = *created;
    result.chatId = *chatId;
    result.participantUserIds = *participantUserIds;
    return result;
}

std::optional<ServerRoomLeftPayload> JsonParser::parseServerRoomLeftPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    const auto left = getJsonField<bool>(payload, "left");
    const auto userId = getJsonField<IDType>(payload, "user-id");
    const auto chatId = getJsonField<IDType>(payload, "chat-id");
    if (!type.has_value() || *type != "room-left" || !left.has_value() || !userId.has_value() || !chatId.has_value())
    {
        return std::nullopt;
    }

    ServerRoomLeftPayload result{};
    result.left = *left;
    result.userId = *userId;
    result.chatId = *chatId;
    return result;
}

std::optional<ServerChatsRequestPayload> JsonParser::parseServerChatsRequestPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    if (!type.has_value() || *type != "chats-payload")
    {
        return std::nullopt;
    }

    const auto chatsIt = payload.find("chats");
    if (chatsIt == payload.end() || !chatsIt->is_object())
    {
        return std::nullopt;
    }

    ServerChatsRequestPayload result{};
    result.type = *type;

    for (const auto& [key, value] : chatsIt->items())
    {
        if (!value.is_string())
        {
            continue;
        }
        try
        {
            const auto chatId = static_cast<IDType>(std::stoul(key));
            result.chats.emplace(chatId, value.get<std::string>());
        }
        catch (...)
        {
            continue;
        }
    }

    return result;
}

std::optional<ServerUsersRequestPayload> JsonParser::parseServerUsersRequestPayload(const nlohmann::json& payload)
{
    const auto type = getJsonField<std::string>(payload, "type");
    if (!type.has_value() || *type != "users-payload")
    {
        return std::nullopt;
    }

    const auto usersIt = payload.find("users");
    if (usersIt == payload.end() || !usersIt->is_object())
    {
        return std::nullopt;
    }

    ServerUsersRequestPayload result{};
    result.type = *type;

    for (const auto& [key, value] : usersIt->items())
    {
        if (!value.is_string())
        {
            continue;
        }
        try
        {
            const auto chatId = static_cast<IDType>(std::stoul(key));
            result.chats.emplace(chatId, value.get<std::string>());
        }
        catch (...)
        {
            continue;
        }
    }

    return result;
}

std::optional<bool> JsonParser::parseServerAlive(const nlohmann::json& payload)
{
    return getJsonField<bool>(payload, "alive");
}

std::optional<std::string> JsonParser::parseServerName(const nlohmann::json& payload)
{
    return getJsonField<std::string>(payload, "server-name");
}

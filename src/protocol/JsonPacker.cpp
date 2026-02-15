#include "protocol/JsonPacker.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string JsonPacker::packServerHello(const ServerHelloPayload& payload)
{
    return json{
        {"type", payload.type},
        {"authorized", payload.authorized},
        {"registration-timeout-seconds", payload.registrationTimeoutSeconds},
        {"server-name", payload.serverName},
    }
        .dump();
}

std::string JsonPacker::packRegistration(const ServerRegistrationPayload& payload)
{
    return json{
        {"registered", payload.registered},
        {"user-id", payload.userId},
        {"server-public-key", payload.serverPublicKey},
        {"users-chats", payload.usersChats},
        {"server-name", payload.serverName},
        {"protocol-version", payload.protocolVersion},
    }
        .dump();
}

std::string JsonPacker::packError(const ServerErrorPayload& payload)
{
    return json{
        {"type", payload.type},
        {"code", payload.code},
        {"message", payload.message},
    }
        .dump();
}

std::string JsonPacker::packChatMessage(const ServerChatMessagePayload& payload)
{
    return json{
        {"type", payload.type},
        {"user-id", payload.userId},
        {"chat-id", payload.chatId},
        {"message", payload.message},
        {"server-message-id", payload.serverMessageId},
    }
        .dump();
}

std::string JsonPacker::packRoomCreated(const ServerRoomCreatedPayload& payload)
{
    return json{
        {"type", payload.type},
        {"created", payload.created},
        {"chat-id", payload.chatId},
        {"participant-user-ids", payload.participantUserIds},
    }
        .dump();
}

std::string JsonPacker::packRoomLeft(const ServerRoomLeftPayload& payload)
{
    return json{
        {"type", payload.type},
        {"left", payload.left},
        {"user-id", payload.userId},
        {"chat-id", payload.chatId},
    }
        .dump();
}

std::string JsonPacker::packServerInfo(bool alive, const std::string& serverName)
{
    return json{
        {"alive", alive},
        {"server-name", serverName},
    }
        .dump();
}


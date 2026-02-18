#include "protocol/JsonPacker.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string JsonPacker::packRegisterRequest(const ClientRegisterRequest &payload)
{
    return json{
        {"type", payload.type},
        {"public-key", payload.publicKey},
        {"password", payload.password},
        {"username", payload.username},
        {"client-version", payload.clientVersion},
    }
        .dump();
}

std::string JsonPacker::packChatMessageRequest(const ClientChatMessageRequest &payload)
{
    return json{
        {"type", payload.type},
        {"user-id", payload.userId},
        {"chat-id", payload.chatId},
        {"message", payload.message},
        {"client-message-id", payload.clientMessageId},
    }
        .dump();
}

std::string JsonPacker::packCreateRoomRequest(const ClientCreateRoomRequest &payload)
{
    return json{
        {"type", payload.type},
        {"user-id", payload.userId},
        {"participant-user-ids", payload.participantUserIds},
        {"is-private", payload.isPrivate},
    }
        .dump();
}

std::string JsonPacker::packLeaveRoomRequest(const ClientLeaveRoomRequest &payload)
{
    return json{
        {"type", payload.type},
        {"user-id", payload.userId},
        {"chat-id", payload.chatId},
    }
        .dump();
}

std::string JsonPacker::packServerHello(const ServerHelloPayload &payload)
{
    return json{
        {"type", payload.type},
        {"authorized", payload.authorized},
        {"registration-timeout-seconds", payload.registrationTimeoutSeconds},
        {"server-name", payload.serverName},
    }
        .dump();
}

std::string JsonPacker::packRegistration(const ServerRegistrationPayload &payload)
{
    json res = json{
        {"type", payload.type},
        {"registered", payload.registered},
        {"user-id", payload.userId},
        {"server-public-key", payload.serverPublicKey},
        {"server-name", payload.serverName},
        {"protocol-version", payload.protocolVersion},
    };

    return res.dump();
}

std::string JsonPacker::packError(const ServerErrorPayload &payload)
{
    return json{
        {"type", payload.type},
        {"code", payload.code},
        {"message", payload.message},
    }
        .dump();
}

std::string JsonPacker::packChatMessage(const ServerChatMessagePayload &payload)
{
    return json{
        {"type", payload.type},
        {"user-id", payload.userId},
        {"username", payload.userName},
        {"chat-id", payload.chatId},
        {"message", payload.message},
        {"server-message-id", payload.serverMessageId},
    }
        .dump();
}

std::string JsonPacker::packRoomCreated(const ServerRoomCreatedPayload &payload)
{
    return json{
        {"type", payload.type},
        {"created", payload.created},
        {"chat-id", payload.chatId},
        {"participant-user-ids", payload.participantUserIds},
    }
        .dump();
}

std::string JsonPacker::packRoomLeft(const ServerRoomLeftPayload &payload)
{
    return json{
        {"type", payload.type},
        {"left", payload.left},
        {"user-id", payload.userId},
        {"chat-id", payload.chatId},
    }
        .dump();
}

std::string JsonPacker::packServerInfo(bool alive, const std::string &serverName)
{
    return json{
        {"alive", alive},
        {"server-name", serverName},
    }
        .dump();
}
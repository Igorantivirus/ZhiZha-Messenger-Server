#pragma once

#include <cstdint>
#include <expected>
#include <variant>

#include "Events.hpp"

namespace protocol::ws
{
using EventFromClient = std::variant<
    ErrorEvent,
    PingEvent,
    PongEvent,
    DispatcinghNewMessageEvent>;
using EventFromServer = std::variant<
    ErrorEvent,
    PingEvent,
    PongEvent,
    SendingNewMessageEvent,
    MessageAckEvent,
    RoomCreatedEvent,
    RoomDeletedEvent,
    RoomUpdatedEvent,
    UserJoinEvent,
    UserLeftEvent,
    UserUpdateEvent>;

class WsEventParser
{
public:
    enum class Error : std::uint8_t
    {
        Unknown,
        EmptyString,
        InvalidTypeValue,
        UnparsableType,
        NotContainType,
        InvalidJson
    };

public:
    static inline std::expected<EventFromServer, Error> parseMessageFromServer(std::string_view str)
    {
        if (str.empty())
            return std::unexpected(Error::EmptyString);
        try
        {
            auto json = nlohmann::json::parse(str);
            if (!json.contains("type"))
                return std::unexpected(Error::NotContainType);
            WsEventType type = WsEventType::Unknown;
            from_json(json["type"], type);
            if (type == WsEventType::Unknown)
                return std::unexpected(Error::UnparsableType);
            switch (type)
            {
            case WsEventType::Error:
                return decodeFromServer<ErrorEvent>(json);
            case WsEventType::Ping:
                return decodeFromServer<PingEvent>(json);
            case WsEventType::Pong:
                return decodeFromServer<PongEvent>(json);
            case WsEventType::SendingNewMessage:
                return decodeFromServer<SendingNewMessageEvent>(json);
            case WsEventType::MessageAck:
                return decodeFromServer<MessageAckEvent>(json);
            case WsEventType::RoomCreated:
                return decodeFromServer<RoomCreatedEvent>(json);
            case WsEventType::RoomDeleted:
                return decodeFromServer<RoomDeletedEvent>(json);
            case WsEventType::RoomUpdated:
                return decodeFromServer<RoomUpdatedEvent>(json);
            case WsEventType::UserJoin:
                return decodeFromServer<UserJoinEvent>(json);
            case WsEventType::UserLeft:
                return decodeFromServer<UserLeftEvent>(json);
            case WsEventType::UserUpdate:
                return decodeFromServer<UserUpdateEvent>(json);
            default:
                return std::unexpected(Error::InvalidTypeValue);
            }
        }
        catch (const nlohmann::json::exception &)
        {
            return std::unexpected(Error::InvalidJson);
        }
        return std::unexpected(Error::Unknown);
    }

    static inline std::expected<EventFromClient, Error> parseMessageFromClient(std::string_view str)
    {
        if (str.empty())
            return std::unexpected(Error::EmptyString);
        try
        {
            auto json = nlohmann::json::parse(str);
            if (!json.contains("type"))
                return std::unexpected(Error::NotContainType);
            WsEventType type = WsEventType::Unknown;
            from_json(json["type"], type);
            if (type == WsEventType::Unknown)
                return std::unexpected(Error::UnparsableType);
            switch (type)
            {
            case WsEventType::Error:
                return decodeFromClient<ErrorEvent>(json);
            case WsEventType::Ping:
                return decodeFromClient<PingEvent>(json);
            case WsEventType::Pong:
                return decodeFromClient<PongEvent>(json);
            case WsEventType::DispatcinghNewMessage:
                return decodeFromClient<DispatcinghNewMessageEvent>(json);
            default:
                return std::unexpected(Error::InvalidTypeValue);
            }
        }
        catch (const nlohmann::json::exception &)
        {
            return std::unexpected(Error::InvalidJson);
        }
        return std::unexpected(Error::Unknown);
    }

    static inline protocol::ErrorCode mapParsingError(Error e)
    {
        using protocol::ErrorCode;
        switch (e)
        {
        case Error::EmptyString:
        case Error::InvalidJson:
            return ErrorCode::InvalidFormat;
        case Error::NotContainType:
        case Error::UnparsableType:
        case Error::InvalidTypeValue:
            return ErrorCode::UnknownMessageType;
        case Error::Unknown:
            break;
        }
        return ErrorCode::InvalidFormat;
    }

private:
    template <typename T>
    static EventFromServer decodeFromServer(const nlohmann::json &j)
    {
        T dto;
        j.get_to(dto);
        return EventFromServer{std::move(dto)};
    }
    template <typename T>
    static EventFromClient decodeFromClient(const nlohmann::json &j)
    {
        T dto;
        j.get_to(dto);
        return EventFromClient{std::move(dto)};
    }
};

} // namespace protocol::ws
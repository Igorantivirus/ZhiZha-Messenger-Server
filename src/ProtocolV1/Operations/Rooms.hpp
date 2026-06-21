#pragma once

#include "Protocol/Types.hpp"
#include <ProtocolV1/Common/Http.hpp>
#include <ProtocolV1/Common/Types.hpp>
#include <ProtocolV1/Dto/Rooms.hpp>

namespace protocol::rooms
{

// Операция получения своих комнат
struct ListRoomsOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "rooms";

    struct PathParams
    {
    };
    struct QueryParams
    {
        RoomId lastLoadedRoomId; // id последений зхагруженной комнаты (по возрастанию)
        unsigned limit;          // Максимальное загружэаемое число комнат за раз
    };

    using Request = void;
    using Response = dto::ListRoomsResponseDto;
};

// Операция создание комнаты
struct CreateRoomOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "rooms";

    struct PathParams
    {
    };
    struct QueryParams
    {
    };

    using Request = dto::CreateRoomRequestDto;
    using Response = dto::CreateRoomResponseDto;
};

// Запрос на получение списка комнат
struct LoopRoomsOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "rooms/loop";

    struct PathParams
    {
    };
    struct QueryParams
    {
        std::string query;
        unsigned limit;
    };

    using Request = void;
    using Response = dto::LoopRoomsResponseDto;
};

// Операция получения комнаты по id
struct GetRoomOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "rooms/<uint>";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = dto::GetRoomResponseDto;
};

// Операция удаления комнаты по id
struct DeleteRoomOperation
{
    static constexpr HttpMethod method = HttpMethod::delete_;
    static constexpr Path path = API "rooms/<uint>";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = void;
};

// Операция обновления настроек комнаты
struct UpdateRoomOperation
{
    static constexpr HttpMethod method = HttpMethod::put;
    static constexpr Path path = API "rooms/<uint>";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = dto::UpdateRoomRequestDto;
    using Response = void;
};

// Операция получения списка участников комнаты
struct ListMembersOperation
{
    static constexpr HttpMethod method = HttpMethod::get;
    static constexpr Path path = API "rooms/<uint>/members";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = dto::ListMembersResponseDto;
};

// Операция пригласить пользователя.
struct InviteMemberOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "rooms/<uint>/members";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = dto::InviteMemberRequestDto;
    using Response = void;
};

// Операция присоединения к комнате
struct JoinRoomOperation
{
    static constexpr HttpMethod method = HttpMethod::post;
    static constexpr Path path = API "rooms/<uint>/members/me";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = void;
};

// Операция выхоад из комнаты
struct LeaveRoomOperation
{
    static constexpr HttpMethod method = HttpMethod::delete_;
    static constexpr Path path = API "rooms/<uint>/members/me";

    struct PathParams
    {
        RoomId roomId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = void;
};

// Операция добавления участника комнаты
struct KickMemberOperation
{
    static constexpr HttpMethod method = HttpMethod::delete_;
    static constexpr Path path = API "rooms/<uint>/members/<uint>";

    struct PathParams
    {
        RoomId roomId;
        UserId userId;
    };
    struct QueryParams
    {
    };

    using Request = void;
    using Response = void;
};

} // namespace protocol::rooms
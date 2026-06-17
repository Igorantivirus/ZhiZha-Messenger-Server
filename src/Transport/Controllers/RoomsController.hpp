#pragma once

#include "ChatService/Types/RoomWithLastMessage.hpp"
#include "Protocol/Types.hpp"
#include "Utils/BindMethod.hpp"
#include "Utils/QueryParamsHelper.hpp"
#include <optional>
#include <ranges>

#include <crow/crow.h>

#include <Auth/AuthService.hpp>
#include <ChatService/ChatService.hpp>
#include <Protocol/ErrorCode.hpp>
#include <Protocol/Rooms.hpp>
#include <Transport/HttpHelpers.hpp>

// /api/v1/rooms/* — CRUD комнат и операции с членством.
// Сообщения — в отдельном MessagesController.
class RoomsController
{
public:
    RoomsController(crow::SimpleApp &app, AuthService &auth, ChatService &chat, IRoomRepository &rooms)
        : app_(app), auth_(auth), chat_(chat), rooms_(rooms)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/rooms").methods("GET"_method)(utils::bindMethod(this, &RoomsController::handleListRooms));   // Список моих комнат
        CROW_ROUTE(app_, "/api/v1/rooms").methods("POST"_method)(utils::bindMethod(this, &RoomsController::handleCreateRoom)); // Создать комнату

        CROW_ROUTE(app_, "/api/v1/rooms/loop").methods("POST"_method)(utils::bindMethod(this, &RoomsController::handleLoopRooms)); // Поиск открытых комнат

        CROW_ROUTE(app_, "/api/v1/rooms/<uint>").methods("GET"_method)(utils::bindMethod(this, &RoomsController::handleGetRoom));       // Детали комнаты
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>").methods("DELETE"_method)(utils::bindMethod(this, &RoomsController::handleDeleteRoom)); // Удалить комнату
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>").methods("PUT"_method)(utils::bindMethod(this, &RoomsController::handleChangeRoom));    // Обновить детали комнаты

        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members").methods("GET"_method)(utils::bindMethod(this, &RoomsController::handleGetMembers));           // Список участников
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members").methods("POST"_method)(utils::bindMethod(this, &RoomsController::handleInviteMember));        // Пригласить участника
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members/me").methods("POST"_method)(utils::bindMethod(this, &RoomsController::handleJoinRoom));         // Войти в комнату
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members/<uint>").methods("DELETE"_method)(utils::bindMethod(this, &RoomsController::handleKickMember)); // Выгнать участника
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members/me").methods("DELETE"_method)(utils::bindMethod(this, &RoomsController::handleLeaveRoom));      // Выйти из комнаты
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;
    ChatService &chat_;
    IRoomRepository &rooms_;

private:
    crow::response handleListRooms(const crow::request &req)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        std::optional<protocol::RoomId> lastLoadedRoomId = utils::parseQuery<protocol::RoomId>(req.url_params.get("lastLoadedRoomId"));
        unsigned limit = utils::parseQuery(req.url_params.get("limit"), 0);

        auto rooms = chat_.getRoomsByUser(*userId, limit + 1, lastLoadedRoomId);
        if (!rooms.has_value())
            return HttpHelpers::mapChatError(rooms.error());

        protocol::rooms::GetRoomsResponse resp;
        resp.hasMore = rooms->size() > limit;

        resp.rooms = rooms.value() | std::views::transform([this](RoomWithLastMessage &rwlm) -> protocol::rooms::RoomInformation
        {
            protocol::rooms::RoomInformation res;

            res.roomInfo.info = std::move(rwlm.room.info);
            res.roomInfo.id = std::move(rwlm.room.id);
            res.roomInfo.name = std::move(rwlm.room.name);

            res.lastMessage.createdAt = std::move(rwlm.msg.createdAt);
            res.lastMessage.fromUserId = std::move(rwlm.msg.fromUserId);
            res.lastMessage.id = std::move(rwlm.msg.id);
            res.lastMessage.text = std::move(rwlm.msg.text);

            res.participantsCount = rwlm.participantsCount;

            return res;
        }) | std::ranges::to<std::vector<protocol::rooms::RoomInformation>>();
        if (resp.hasMore)
            resp.rooms.pop_back();

        for (const auto &room : resp.rooms)
        {
            if (resp.postMessageSenders.contains(room.lastMessage.fromUserId))
                continue;
            auto user = auth_.getUserRepository().findUserById(room.lastMessage.fromUserId);
            if (!user)
                continue;
            protocol::users::UserDisplayInfo udi;
            udi.info.birthDate = user->birthDate;
            udi.info.country = user->country;
            udi.registerTime = user->registerTime;
            udi.info.displayname = std::move(user->displayeName);
            udi.info.username = std::move(user->username);
            resp.postMessageSenders[room.lastMessage.fromUserId] = std::move(udi);
        }

        return HttpHelpers::jsonResponse(200, resp);
    }

    crow::response handleCreateRoom(const crow::request &req)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        protocol::rooms::CreateRoomRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        auto roomId = chat_.createRoom(*userId, dto);
        if (!roomId.has_value())
            return HttpHelpers::mapChatError(roomId.error());

        protocol::rooms::CreateRoomResponse resp{.roomId = *roomId};
        return HttpHelpers::jsonResponse(201, resp);
    }

    crow::response handleLoopRooms(const crow::request &req)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        std::optional<std::string> query = utils::parseQuery<std::string>(req.url_params.get("query"));
        std::optional<unsigned> limit = utils::parseQuery<unsigned>(req.url_params.get("limit"));

        if (!query || !limit)
            return HttpHelpers::errorResponse(400, protocol::ErrorCode::MissingParams, "Missing params: query or limit.");

        // Пустой результат — это валидный ответ (пустой массив уходит в сеть как есть).
        auto rooms = rooms_.getRoomsByQuery(*query, *limit);

        protocol::rooms::RoomsLoopByExampleResponse resp;
        for (auto &&room : rooms)
        {
            protocol::rooms::Room roomDto;
            roomDto.id = std::move(room.id);
            roomDto.info = std::move(room.info);
            roomDto.name = std::move(room.name);
            resp.users[roomDto.id] = std::move(roomDto);
        }
        return HttpHelpers::jsonResponse(200, resp);
    }

    crow::response handleGetRoom(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto roomInfo = chat_.getRoomInfo(*userId, roomId);
        if (!roomInfo)
            return HttpHelpers::mapChatError(roomInfo.error());

        protocol::rooms::RoomInfoResponse resp;
        resp.room.info = roomInfo->info;
        resp.room.id = roomInfo->id;
        resp.room.name = roomInfo->name;
        return HttpHelpers::jsonResponse(200, resp);
    }

    crow::response handleDeleteRoom(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        // Проверка прав (только Owner), удаление с CASCADE и WS-рассылка — в домене.
        if (auto error = chat_.deleteRoom(*userId, roomId))
            return HttpHelpers::mapChatError(error.value());

        return crow::response(204);
    }

    crow::response handleChangeRoom(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        protocol::rooms::ChangeRoomRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        // Проверка прав, запись и WS-рассылка участникам — внутри домена.
        if (auto error = chat_.changeRoomInfo(*userId, roomId, std::move(dto.newRoomInfo)))
            return HttpHelpers::mapChatError(error.value());

        return crow::response(204);
    }

    crow::response handleGetMembers(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto members = chat_.getMembers(*userId, roomId);
        if (!members)
            return HttpHelpers::mapChatError(members.error());

        protocol::rooms::RoomsMembersInfoResponse resp;
        resp.members = members.value() | std::views::transform([this](const RoomMember &member) -> protocol::rooms::Member
        {
            // Добираем display-инфо по id (как в handleListRooms). Имена живут
            // в Auth, поэтому домен их не знает — собираем здесь, в транспорте.
            protocol::users::UserDisplayInfo display;
            if (auto user = auth_.getUserRepository().findUserById(member.userId))
            {
                display.info.birthDate = user->birthDate;
                display.info.country = user->country;
                display.registerTime = user->registerTime;
                display.info.username = std::move(user->username);
                display.info.displayname = std::move(user->displayeName);
            }
            return protocol::rooms::Member{
                .userId = member.userId,
                .display = std::move(display),
                .role = member.role};
        }) | std::ranges::to<std::vector<protocol::rooms::Member>>();

        return HttpHelpers::jsonResponse(200, resp);
    }

    crow::response handleInviteMember(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        protocol::rooms::InviteMemberRequest dto;
        if (!HttpHelpers::parseBody(req, dto))
            return HttpHelpers::invalidFormatResponse();

        if (!auth_.getUserRepository().findUserById(dto.invitedId))
            return HttpHelpers::errorResponse(404, protocol::ErrorCode::NotFound, "The invited user was not found");

        if (auto error = chat_.inviteUser(*userId, roomId, dto.invitedId))
            return HttpHelpers::mapChatError(error.value());

        return crow::response(204);
    }

    crow::response handleJoinRoom(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        // Самоприсоединение к публичной комнате: проверка joinPolicy и WS — в домене.
        if (auto error = chat_.joinRoom(*userId, roomId))
            return HttpHelpers::mapChatError(error.value());

        return crow::response(204);
    }

    crow::response handleKickMember(const crow::request &req, std::uint64_t roomId, std::uint64_t memberId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        // memberId берём из URL (источник истины). Проверка прав и WS — в домене.
        if (auto error = chat_.kickFromRoom(*userId, memberId, roomId))
            return HttpHelpers::mapChatError(error.value());

        return crow::response(204);
    }

    crow::response handleLeaveRoom(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        if (auto error = chat_.leaveRoom(*userId, roomId))
            return HttpHelpers::mapChatError(error.value());

        return crow::response(204);
    }
};
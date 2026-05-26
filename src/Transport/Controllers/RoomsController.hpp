#pragma once

#include "ChatService/Types/RoomWithLastMessage.hpp"
#include "Protocol/Types.hpp"
#include "Utils/BindMethod.hpp"
#include <cstdlib>
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
    RoomsController(crow::SimpleApp &app, AuthService &auth, ChatService &chat)
        : app_(app), auth_(auth), chat_(chat)
    {
    }

    void registerRoutes()
    {
        CROW_ROUTE(app_, "/api/v1/rooms").methods("GET"_method)(utils::bindMethod(this, &RoomsController::handleListRooms));                      // Список моих комнат
        CROW_ROUTE(app_, "/api/v1/rooms").methods("POST"_method)(utils::bindMethod(this, &RoomsController::handleCreateRoom));                    // Создать комнату
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>")(utils::bindMethod(this, &RoomsController::handleGetRoom));                                       // Детали комнаты
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members").methods("GET"_method)(utils::bindMethod(this, &RoomsController::handleGetMembers));      // Список участников
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members").methods("POST"_method)(utils::bindMethod(this, &RoomsController::handleInviteMember));   // Пригласить участника
        CROW_ROUTE(app_, "/api/v1/rooms/<uint>/members/me").methods("DELETE"_method)(utils::bindMethod(this, &RoomsController::handleLeaveRoom)); // Покинуть комнату
    }

private:
    crow::SimpleApp &app_;
    AuthService &auth_;
    ChatService &chat_;

private:
    crow::response handleListRooms(const crow::request &req)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto lastLoadedRoomId = req.url_params.get("lastLoadedRoomId"); // const char* или nullptr
        auto limit = req.url_params.get("limit");

        int intLimit = std::atoi(limit);

        auto rooms = chat_.getRoomsByUser(
            *userId,
            intLimit + 1,
            lastLoadedRoomId
                ? std::optional<protocol::RoomId>(std::atoll(lastLoadedRoomId))
                : std::nullopt);
        if (!rooms.has_value())
            return HttpHelpers::mapChatError(rooms.error());

        protocol::rooms::GetRoomsResponse resp;
        resp.hasMore = rooms->size() > intLimit;

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

            return res;
        }) | std::ranges::to<std::vector<protocol::rooms::RoomInformation>>();
        if (resp.hasMore)
            resp.rooms.pop_back();

        for(const auto& room : resp.rooms)
        {
            if(resp.postMessageSenders.contains(room.lastMessage.fromUserId))
                continue;
            auto user = auth_.getUserRepository().findUserById(room.lastMessage.fromUserId);
            if(!user)
                continue;
            protocol::users::UserDisplayInfo udi;
            udi.displayname = std::move(user->displayeName);
            udi.username = std::move(user->username);
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

    crow::response handleGetMembers(const crow::request &req, std::uint64_t roomId)
    {
        auto userId = HttpHelpers::requireAuth(req, auth_);
        if (!userId)
            return HttpHelpers::unauthorizedResponse();

        auto members = chat_.getMembers(*userId, roomId);
        if (!members)
            return HttpHelpers::mapChatError(members.error());

        protocol::rooms::RoomsMembersInfoResponse resp;
        resp.members = members.value() | std::views::transform([](const RoomMember &member) -> protocol::rooms::Member
        {
            return protocol::rooms::Member{.userId = member.userId};
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
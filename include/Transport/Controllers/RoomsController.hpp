#pragma once

#include "SessionService/SessionService.hpp"
#include <crow/crow.h>

#include <AuthService/AuthService.hpp>
#include <AuthService/UserQueryService.hpp>
#include <ChatService/ChatQueryService.hpp>
#include <ChatService/ChatService.hpp>
#include <Protocol/Operations/Rooms.hpp>
#include <Transport/Helpers.hpp>

namespace transport
{

class RoomsController
{
public:
    RoomsController(
        crow::SimpleApp &app,
        session::SessionService& session_,
        auth::AuthService &auth,
        chat::ChatService &chat,
        auth::UserQueryService &usersQuery,
        chat::ChatQueryService &chatQuery);

    void registerRoutes();

private:
    crow::SimpleApp &app_;
    session::SessionService& session_;

    auth::AuthService &auth_;
    chat::ChatService &chat_;

    auth::UserQueryService &usersQuery_;
    chat::ChatQueryService &chatQuery_;

private:
    using ListRooms = protocol::rooms::ListRoomsOperation;
    using CreateRoom = protocol::rooms::CreateRoomOperation;
    using LoopRooms = protocol::rooms::LoopRoomsOperation;
    using GetRoom = protocol::rooms::GetRoomOperation;
    using DeleteRoom = protocol::rooms::DeleteRoomOperation;
    using UpdateRoom = protocol::rooms::UpdateRoomOperation;
    using ListMembers = protocol::rooms::ListMembersOperation;
    using InviteMember = protocol::rooms::InviteMemberOperation;
    using JoinRoom = protocol::rooms::JoinRoomOperation;
    using LeaveRoom = protocol::rooms::LeaveRoomOperation;
    using KickMember = protocol::rooms::KickMemberOperation;

private:
    Helpers::HttpResponse<ListRooms, chat::ChatError> handleListRooms(Helpers::HttpRequest<ListRooms> req);
    Helpers::HttpResponse<CreateRoom, chat::ChatError> handleCreateRoom(Helpers::HttpRequest<CreateRoom> req);
    Helpers::HttpResponse<LoopRooms, chat::ChatError> handleLoopRooms(Helpers::HttpRequest<LoopRooms> req);
    Helpers::HttpResponse<GetRoom, chat::ChatError> handleGetRoom(Helpers::HttpRequest<GetRoom> req);
    Helpers::HttpResponse<DeleteRoom, chat::ChatError> handleDeleteRoom(Helpers::HttpRequest<DeleteRoom> req);
    Helpers::HttpResponse<UpdateRoom, chat::ChatError> handleUpdateRoom(Helpers::HttpRequest<UpdateRoom> req);
    Helpers::HttpResponse<ListMembers, chat::ChatError> handleListMembers(Helpers::HttpRequest<ListMembers> req);
    Helpers::HttpResponse<InviteMember, chat::ChatError> handleInviteMember(Helpers::HttpRequest<InviteMember> req);
    Helpers::HttpResponse<JoinRoom, chat::ChatError> handleJoinRoom(Helpers::HttpRequest<JoinRoom> req);
    Helpers::HttpResponse<LeaveRoom, chat::ChatError> handleLeaveRoom(Helpers::HttpRequest<LeaveRoom> req);
    Helpers::HttpResponse<KickMember, chat::ChatError> handleKickMember(Helpers::HttpRequest<KickMember> req);
};

} // namespace transport
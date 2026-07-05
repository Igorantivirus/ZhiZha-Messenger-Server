#include "Protocol/Ws/Events.hpp"
#include <Transport/Controllers/RoomsController.hpp>

#include <unordered_map>
#include <utility>
#include <vector>

#include <ChatService/ChatService.hpp>
#include <Protocol/Data/Rooms.hpp>
#include <Transport/Helpers.hpp>
#include <Utils/Types.hpp>

namespace transport
{

namespace
{
protocol::data::RoomWithLastMessage toDto(chat::RoomForUser room)
{
    protocol::data::RoomWithLastMessage res;
    res.roomInfo.info = std::move(room.room.info);
    res.roomInfo.id = std::move(room.room.id);
    res.roomInfo.name = std::move(room.room.name);

    res.participantsCount = std::move(room.participantsCount);
    res.senderRole = std::move(room.role);

    res.lastMessage.roomId = std::move(room.lastMessage.roomId);
    res.lastMessage.createdAt = std::move(room.lastMessage.createdAt);
    res.lastMessage.fromUserId = std::move(room.lastMessage.fromUserId);
    res.lastMessage.text = std::move(room.lastMessage.text);
    res.lastMessage.id = std::move(room.lastMessage.id);

    return res;
}

protocol::data::UserDisplayInfo toDto(auth::UserPublicView view)
{
    protocol::data::UserDisplayInfo res;
    res.displayName = std::move(view.displayName);
    return res;
}

protocol::data::Room toDto(chat::Room room)
{
    protocol::data::Room res;
    res.id = room.id;
    res.name = std::move(room.name);
    res.info = room.info;
    return res;
}

} // namespace

RoomsController::RoomsController(
    crow::SimpleApp &app,
    session::SessionService &session,
    auth::AuthService &auth,
    chat::ChatService &chat,
    auth::UserQueryService &usersQuery,
    chat::ChatQueryService &chatQuery)
    : app_(app),
      session_(session),
      auth_(auth),
      chat_(chat),
      usersQuery_(usersQuery),
      chatQuery_(chatQuery)
{
}

void RoomsController::registerRoutes()
{
    Helpers::bindWithAuth<ListRooms>(app_, this, auth_, &RoomsController::handleListRooms);
    Helpers::bindWithAuth<CreateRoom>(app_, this, auth_, &RoomsController::handleCreateRoom);
    Helpers::bindWithAuth<LoopRooms>(app_, this, auth_, &RoomsController::handleLoopRooms);
    Helpers::bindWithAuth<GetRoom>(app_, this, auth_, &RoomsController::handleGetRoom);
    Helpers::bindWithAuth<DeleteRoom>(app_, this, auth_, &RoomsController::handleDeleteRoom);
    Helpers::bindWithAuth<UpdateRoom>(app_, this, auth_, &RoomsController::handleUpdateRoom);
    Helpers::bindWithAuth<ListMembers>(app_, this, auth_, &RoomsController::handleListMembers);
    Helpers::bindWithAuth<InviteMember>(app_, this, auth_, &RoomsController::handleInviteMember);
    Helpers::bindWithAuth<JoinRoom>(app_, this, auth_, &RoomsController::handleJoinRoom);
    Helpers::bindWithAuth<LeaveRoom>(app_, this, auth_, &RoomsController::handleLeaveRoom);
    Helpers::bindWithAuth<KickMember>(app_, this, auth_, &RoomsController::handleKickMember);
}

Helpers::HttpResponse<RoomsController::ListRooms, chat::ChatError> RoomsController::handleListRooms(Helpers::HttpRequest<ListRooms> req)
{
    auto res = chatQuery_.getRoomsOfUser(*req.userId, req.query.afterId, req.query.limit);
    if (!res)
        return std::unexpected(res.error());

    ListRooms::Response resp;
    resp.hasMore = res->hasMore;

    std::vector<utils::UserId> lastSendersId;

    for (auto &&room : res->elems)
    {
        lastSendersId.push_back(room.lastMessage.fromUserId);
        resp.rooms.push_back(std::move(toDto(std::move(room))));
    }

    auto infos = usersQuery_.getDisplayInfos(lastSendersId);

    for (auto &&info : infos)
        resp.postMessageSenders[info.id] = toDto(std::move(info));

    return resp;
}

Helpers::HttpResponse<RoomsController::CreateRoom, chat::ChatError> RoomsController::handleCreateRoom(Helpers::HttpRequest<CreateRoom> req)
{
    auto res = chat_.createRoom(
        *req.userId,
        req.body.roomName,
        req.body.roomInfo,
        req.body.invitedUsers);
    if (!res)
        return std::unexpected(res.error());

    // Рассылка: приглашённые + сам creator. Инициатор тоже в списке,
    // так как у него могут быть другие открытые устройства — им тоже
    // надо узнать о новой комнате.
    protocol::ws::RoomCreatedEvent rce;
    rce.room.id = *res;
    rce.room.info = req.body.roomInfo;
    rce.room.name = req.body.roomName;

    std::vector<utils::UserId> recipients = req.body.invitedUsers;
    recipients.push_back(*req.userId);
    session_.sendToUsers(recipients, Helpers::dtoToString(rce));

    CreateRoom::Response resp;
    resp.roomId = *res;
    return resp;
}

Helpers::HttpResponse<RoomsController::LoopRooms, chat::ChatError> RoomsController::handleLoopRooms(Helpers::HttpRequest<LoopRooms> req)
{
    auto rooms = chatQuery_.searchRooms(req.query.query, req.query.limit);

    LoopRooms::Response resp;
    resp.rooms.reserve(rooms.size());
    for (auto &&room : rooms)
    {
        const auto id = room.id;
        resp.rooms.emplace(id, toDto(std::move(room)));
    }
    return resp;
}

Helpers::HttpResponse<RoomsController::GetRoom, chat::ChatError> RoomsController::handleGetRoom(Helpers::HttpRequest<GetRoom> req)
{
    auto res = chatQuery_.getRoom(*req.userId, req.path.roomId);
    if (!res)
        return std::unexpected(res.error());

    GetRoom::Response resp;
    resp.room.id = res->id;
    resp.room.name = std::move(res->name);
    resp.room.info = res->info;
    return resp;
}

Helpers::HttpResponse<RoomsController::DeleteRoom, chat::ChatError> RoomsController::handleDeleteRoom(Helpers::HttpRequest<DeleteRoom> req)
{
    auto res = chat_.deleteRoom(*req.userId, req.path.roomId);
    if (!res)
        return std::unexpected(res.error());

    protocol::ws::RoomDeletedEvent rde;
    rde.roomId = req.path.roomId;
    session_.sendToUsers(*res, Helpers::dtoToString(rde));
    return {};
}

Helpers::HttpResponse<RoomsController::UpdateRoom, chat::ChatError> RoomsController::handleUpdateRoom(Helpers::HttpRequest<UpdateRoom> req)
{
    auto res = chat_.updateRoom(
        *req.userId,
        req.path.roomId,
        req.body.newInfo,
        req.body.newName);
    if (!res)
        return std::unexpected(res.error());
    protocol::ws::RoomUpdatedEvent rue;
    rue.room.id = req.path.roomId;
    rue.room.info = req.body.newInfo;
    rue.room.name = req.body.newName;
    session_.sendToUsers(*res, Helpers::dtoToString(rue));
    return {};
}

Helpers::HttpResponse<RoomsController::ListMembers, chat::ChatError> RoomsController::handleListMembers(Helpers::HttpRequest<ListMembers> req)
{
    auto res = chatQuery_.getMembers(*req.userId, req.path.roomId, req.query.afterId, req.query.limit);
    if (!res)
        return std::unexpected(res.error());

    ListMembers::Response resp;
    resp.hasMore = res->hasMore;

    // Кросс-доменный join (chat → auth) — по правилу собираем в транспорте.
    std::vector<utils::UserId> ids;
    ids.reserve(res->elems.size());
    for (const auto &m : res->elems)
        ids.push_back(m.userId);

    auto displays = usersQuery_.getDisplayInfos(ids);
    std::unordered_map<utils::UserId, protocol::data::UserDisplayInfo> displayById;
    displayById.reserve(displays.size());
    for (auto &&d : displays)
        displayById.emplace(d.id, toDto(std::move(d)));

    resp.members.reserve(res->elems.size());
    for (const auto &m : res->elems)
    {
        protocol::data::Member dto;
        dto.userId = m.userId;
        dto.role = m.role;
        if (auto it = displayById.find(m.userId); it != displayById.end())
            dto.display = it->second;
        resp.members.push_back(std::move(dto));
    }

    return resp;
}

Helpers::HttpResponse<RoomsController::InviteMember, chat::ChatError> RoomsController::handleInviteMember(Helpers::HttpRequest<InviteMember> req)
{
    auto res = chat_.inviteToRoom(*req.userId, req.body.invitedId, req.path.roomId);
    if (!res)
        return std::unexpected(res.error());
    protocol::ws::UserJoinEvent uje;
    uje.userId = req.body.invitedId;
    uje.roomId = req.path.roomId;
    session_.sendToUsers(*res, Helpers::dtoToString(uje));
    return {};
}

Helpers::HttpResponse<RoomsController::JoinRoom, chat::ChatError> RoomsController::handleJoinRoom(Helpers::HttpRequest<JoinRoom> req)
{
    auto res = chat_.joinToRoom(*req.userId, req.path.roomId);
    if (!res)
        return std::unexpected(res.error());
    protocol::ws::UserJoinEvent uje;
    uje.userId = *req.userId;
    uje.roomId = req.path.roomId;
    session_.sendToUsers(*res, Helpers::dtoToString(uje));
    return {};
}

Helpers::HttpResponse<RoomsController::LeaveRoom, chat::ChatError> RoomsController::handleLeaveRoom(Helpers::HttpRequest<LeaveRoom> req)
{
    auto res = chat_.leaveFromRoom(*req.userId, req.path.roomId);
    if (!res)
        return std::unexpected(res.error());
    protocol::ws::UserLeftEvent ule;
    ule.userId = *req.userId;
    ule.roomId = req.path.roomId;
    session_.sendToUsers(*res, Helpers::dtoToString(ule));
    return {};
}

Helpers::HttpResponse<RoomsController::KickMember, chat::ChatError> RoomsController::handleKickMember(Helpers::HttpRequest<KickMember> req)
{
    auto res = chat_.kickFromRoom(*req.userId, req.path.userId, req.path.roomId);
    if (!res)
        return std::unexpected(res.error());
    protocol::ws::UserLeftEvent ule;
    ule.userId = req.path.userId;
    ule.roomId = req.path.roomId;
    session_.sendToUsers(*res, Helpers::dtoToString(ule));
    return {};
}

} // namespace transport

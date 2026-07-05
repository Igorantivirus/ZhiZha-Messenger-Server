#include <ChatService/ChatService.hpp>

#include <utility>

#include <ChatService/Subservices/MembersRepository.hpp>
#include <ChatService/Subservices/MessageRepository.hpp>
#include <ChatService/Subservices/RoomPermissions.hpp>
#include <ChatService/Subservices/RoomRepository.hpp>

namespace chat
{

namespace
{

// Максимальная длина текста сообщения. Локальная константа сервиса — если
// понадобится настраиваемость, вынесем в конфиг. Пока — жёстко.
constexpr std::size_t MAX_MESSAGE_LENGTH = 4096;

// Максимальная длина имени комнаты — аналогично.
constexpr std::size_t MAX_ROOM_NAME_LENGTH = 128;

} // namespace

ChatService::ChatService(RoomRepository &rooms,
                         MessageRepository &messages,
                         MembersRepository &members)
    : rooms_(rooms), messages_(messages), members_(members)
{
}

namespace
{

// Собрать всех участников комнаты (ids) — для Notifiables.
ChatService::Notifiables snapshotMemberIds(const MembersRepository &members,
                                            utils::RoomId roomId)
{
    return members.findMemberIdsInRoom(roomId);
}

} // namespace

std::expected<utils::RoomId, ChatError> ChatService::createRoom(
    utils::UserId creator,
    std::string name,
    utils::RoomInfo info,
    std::vector<utils::UserId> invitedUsers)
{
    if (name.empty())
        return std::unexpected(ChatError::EmptyRoomName);
    if (name.size() > MAX_ROOM_NAME_LENGTH)
        return std::unexpected(ChatError::EmptyRoomName);

    // Direct — жёсткие инварианты: ровно 1 приглашённый, не сам себе.
    // joinPolicy/writePolicy для direct мы НЕ переопределяем сами (по решению #2):
    // если клиент прислал не те — это ошибка от клиента.
    if (info.kind == utils::RoomKind::Direct)
    {
        if (invitedUsers.size() != 1)
            return std::unexpected(ChatError::InvalidDirectRoom);
        if (invitedUsers[0] == creator)
            return std::unexpected(ChatError::InvalidDirectRoom);
        if (info.joinPolicy != utils::JoinPolicy::Closed)
            return std::unexpected(ChatError::InvalidDirectRoom);
        if (info.writePolicy != utils::WritePolicy::Everyone)
            return std::unexpected(ChatError::InvalidDirectRoom);
    }

    // Три записи, каждая отдельным вызовом репо (см. решение #1).
    // Атомарности нет — если между шагами упадём, будет ремонт при старте.
    // TODO(startup-consistency): при запуске сервера проходить по rooms и
    // удалять «сиротские» — те, у которых нет ни одного участника, или
    // roomMembers без соответствующей rooms-строки.
    auto room = rooms_.create(name, info);

    members_.create(room.id, creator, utils::MemberRole::Owner);
    for (const auto invited : invitedUsers)
    {
        // Проверка существования invited-пользователей — задача транспорта
        // (решение #4). Здесь просто создаём membership.
        // Роль по умолчанию — Member.
        members_.create(room.id, invited, utils::MemberRole::Member);
    }

    return room.id;
}

std::expected<ChatService::Notifiables, ChatError> ChatService::deleteRoom(
    utils::UserId userId,
    utils::RoomId roomId)
{
    auto member = members_.findByIds(roomId, userId);
    if (!member)
        return std::unexpected(ChatError::PermissionError);

    if (!RoomPermissions::canDeleteRoom(member->role))
        return std::unexpected(ChatError::PermissionError);

    // Собираем notifiables ДО удаления — потом roomMembers исчезнут.
    auto notifiables = snapshotMemberIds(members_, roomId);

    // Порядок: сообщения -> участники -> сама комната. Каждый шаг —
    // самостоятельный. FK CASCADE в схеме сделал бы это одним rooms.remove,
    // но, по решению #1, не полагаемся на детали SQL из сервиса.
    messages_.removeAllInRoom(roomId);
    members_.removeAllInRoom(roomId);
    rooms_.remove(roomId);

    return notifiables;
}

std::expected<ChatService::Notifiables, ChatError> ChatService::updateRoom(
    utils::UserId userId,
    utils::RoomId roomId,
    utils::RoomInfo info,
    std::string roomName)
{
    if (roomName.empty())
        return std::unexpected(ChatError::EmptyRoomName);
    if (roomName.size() > MAX_ROOM_NAME_LENGTH)
        return std::unexpected(ChatError::EmptyRoomName);

    auto member = members_.findByIds(roomId, userId);
    if (!member)
        return std::unexpected(ChatError::PermissionError);

    if (!RoomPermissions::canUpdateRoom(member->role))
        return std::unexpected(ChatError::PermissionError);

    // Существование комнаты нужно только чтобы отличить «нет прав» от
    // «нет комнаты». members.findByIds уже сказал, что вызывающий состоит
    // в комнате => комната существует. Отдельный rooms.findById не нужен.

    // Смена типа на Direct возможна только если участников ровно двое
    // (см. решение #3).
    if (info.kind == utils::RoomKind::Direct)
    {
        const auto count = members_.findCountMembersById(roomId);
        if (count != 2)
            return std::unexpected(ChatError::InvalidDirectRoom);
        if (info.joinPolicy != utils::JoinPolicy::Closed)
            return std::unexpected(ChatError::InvalidDirectRoom);
        if (info.writePolicy != utils::WritePolicy::Everyone)
            return std::unexpected(ChatError::InvalidDirectRoom);
    }

    rooms_.updateRoom(roomId, roomName, info);

    return snapshotMemberIds(members_, roomId);
}

// --------------------------- member commands -------------------------------

std::expected<ChatService::Notifiables, ChatError> ChatService::inviteToRoom(
    utils::UserId invitorId,
    utils::UserId invitedId,
    utils::RoomId roomId)
{
    if (invitorId == invitedId)
        return std::unexpected(ChatError::MemberAlready);

    auto invitor = members_.findByIds(roomId, invitorId);
    if (!invitor)
        return std::unexpected(ChatError::PermissionError);

    auto room = rooms_.findById(roomId);
    if (!room)
        return std::unexpected(ChatError::RoomNotFound);

    if (!RoomPermissions::canInvite(invitor->role, room->info.joinPolicy))
        return std::unexpected(ChatError::PermissionError);

    if (room->info.kind == utils::RoomKind::Direct)
    {
        // Direct — приглашения запрещены (там всегда ровно двое).
        return std::unexpected(ChatError::InvalidDirectRoom);
    }

    if (members_.isMember(roomId, invitedId))
        return std::unexpected(ChatError::MemberAlready);

    members_.create(roomId, invitedId, utils::MemberRole::Member);

    return snapshotMemberIds(members_, roomId);
}

std::expected<ChatService::Notifiables, ChatError> ChatService::joinToRoom(
    utils::UserId userId,
    utils::RoomId roomId)
{
    auto room = rooms_.findById(roomId);
    if (!room)
        return std::unexpected(ChatError::RoomNotFound);

    if (!RoomPermissions::canJoin(room->info.joinPolicy))
        return std::unexpected(ChatError::PermissionError);

    if (room->info.kind == utils::RoomKind::Direct)
        return std::unexpected(ChatError::InvalidDirectRoom);

    if (members_.isMember(roomId, userId))
        return std::unexpected(ChatError::MemberAlready);

    members_.create(roomId, userId, utils::MemberRole::Member);

    return snapshotMemberIds(members_, roomId);
}

std::expected<ChatService::Notifiables, ChatError> ChatService::kickFromRoom(
    utils::UserId kickerId,
    utils::UserId kickedId,
    utils::RoomId roomId)
{
    if (kickerId == kickedId)
        return std::unexpected(ChatError::PermissionError);

    auto kicker = members_.findByIds(roomId, kickerId);
    if (!kicker)
        return std::unexpected(ChatError::PermissionError);

    auto kicked = members_.findByIds(roomId, kickedId);
    if (!kicked)
        return std::unexpected(ChatError::NotAMember);

    if (!RoomPermissions::canKick(kicker->role, kicked->role))
        return std::unexpected(ChatError::PermissionError);

    // Notifiables — все текущие участники ДО кика. Включает кикнутого,
    // чтобы он получил «вас выгнали».
    auto notifiables = snapshotMemberIds(members_, roomId);

    members_.removeById(roomId, kickedId);

    return notifiables;
}

std::expected<ChatService::Notifiables, ChatError> ChatService::leaveFromRoom(
    utils::UserId userId,
    utils::RoomId roomId)
{
    auto member = members_.findByIds(roomId, userId);
    if (!member)
        return std::unexpected(ChatError::PermissionError);

    if (!RoomPermissions::canLeave(member->role))
        return std::unexpected(ChatError::PermissionError);

    // Notifiables — все текущие участники ДО ухода. Включает уходящего,
    // чтобы транспорт при желании отдал ему подтверждение.
    auto notifiables = snapshotMemberIds(members_, roomId);

    members_.removeById(roomId, userId);

    return notifiables;
}

// -------------------------- message commands -------------------------------

std::expected<ChatService::SendMessageResponse, ChatError> ChatService::sendMessage(
    utils::UserId userId,
    utils::RoomId roomId,
    std::string message)
{
    if (message.empty())
        return std::unexpected(ChatError::EmptyMessage);
    if (message.size() > MAX_MESSAGE_LENGTH)
        return std::unexpected(ChatError::MessageTooLong);

    auto member = members_.findByIds(roomId, userId);
    if (!member)
        return std::unexpected(ChatError::NotAMember);

    auto room = rooms_.findById(roomId);
    if (!room)
        return std::unexpected(ChatError::RoomNotFound);

    if (!RoomPermissions::canSendMessage(member->role, room->info.writePolicy))
        return std::unexpected(ChatError::WriteForbidden);

    auto msg = messages_.create(roomId, userId, message);

    return SendMessageResponse{
        .messageId = msg.id,
        .notifiables = snapshotMemberIds(members_, roomId),
        .createdAt = msg.createdAt};
}

} // namespace chat

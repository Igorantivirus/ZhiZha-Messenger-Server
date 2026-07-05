#include <ChatService/ChatQueryService.hpp>

#include <utility>

namespace chat
{

ChatQueryService::ChatQueryService(const RoomRepository &rooms,
                                   const MessageRepository &messages,
                                   const MembersRepository &members)
    : rooms_(rooms), messages_(messages), members_(members)
{
}

std::expected<Room, ChatError> ChatQueryService::getRoom(utils::UserId requesterId,
                                                          utils::RoomId roomId) const
{
    // Проверяем сущещствование, потом членство. RoomId не секрет —
    // отдаём PermissionError отдельно от RoomNotFound (решение #6/F).
    auto room = rooms_.findById(roomId);
    if (!room)
        return std::unexpected(ChatError::RoomNotFound);

    if (!members_.isMember(roomId, requesterId))
        return std::unexpected(ChatError::PermissionError);

    return std::move(*room);
}

std::expected<ChatQueryService::Page<RoomForUser>, ChatError> ChatQueryService::getRoomsOfUser(
    utils::UserId userId,
    utils::RoomId afterId,
    unsigned limit) const
{
    // TODO(perf): сейчас N+1 запрос на страницу — обсудили отдельно.
    // Оставлено как есть для читаемости кода первой версии.
    const auto ids = members_.findRoomIdsForUserAfter(userId, limit + 1, afterId);

    const bool hasMore = ids.size() > limit;
    const std::size_t taken = hasMore ? limit : ids.size();

    Page<RoomForUser> page;
    page.elems.reserve(taken);
    page.hasMore = hasMore;

    for (std::size_t i = 0; i < taken; ++i)
    {
        const auto roomId = ids[i];

        auto room = rooms_.findById(roomId);
        if (!room)
            continue; // сиротская запись — пропускаем, TODO под ремонт БД

        auto member = members_.findByIds(roomId, userId);
        if (!member)
            continue; // тоже несогласованность

        const auto count = members_.findCountMembersById(roomId);

        // Пустое сообщение (id=0) означает «сообщений в комнате нет».
        // См. решение #8 — конвенция id=0 = «нет объекта».
        Message lastMessage{};
        if (auto last = messages_.findLastMessageInRoom(roomId))
            lastMessage = std::move(*last);

        page.elems.push_back(RoomForUser{
            .room = std::move(*room),
            .lastMessage = std::move(lastMessage),
            .role = member->role,
            .participantsCount = count});
    }

    return page;
}

std::expected<ChatQueryService::Page<Message>, ChatError> ChatQueryService::getMessages(
    utils::UserId requesterId,
    utils::RoomId roomId,
    std::optional<utils::MessageId> afterId,
    std::optional<utils::MessageId> beforeId,
    unsigned limit) const
{
    if (!members_.isMember(roomId, requesterId))
    {
        // Разделяем «нет комнаты» и «нет прав» — по решению #6.
        if (!rooms_.findById(roomId))
            return std::unexpected(ChatError::RoomNotFound);
        return std::unexpected(ChatError::PermissionError);
    }

    // Три режима курсора: after / before / latest. Одновременно after и before
    // не поддерживаются — в этом случае приоритет у after (произвольный выбор,
    // клиент не должен присылать обе границы).
    std::vector<Message> rows;
    if (afterId)
        rows = messages_.findAfter(roomId, limit + 1, *afterId);
    else if (beforeId)
        rows = messages_.findBefore(roomId, limit + 1, *beforeId);
    else
        rows = messages_.findLatest(roomId, limit + 1);

    const bool hasMore = rows.size() > limit;
    if (hasMore)
        rows.pop_back();

    return Page<Message>{.elems = std::move(rows), .hasMore = hasMore};
}

std::expected<ChatQueryService::Page<Member>, ChatError> ChatQueryService::getMembers(
    utils::UserId requesterId,
    utils::RoomId roomId,
    utils::UserId afterId,
    unsigned limit) const
{
    if (!members_.isMember(roomId, requesterId))
    {
        if (!rooms_.findById(roomId))
            return std::unexpected(ChatError::RoomNotFound);
        return std::unexpected(ChatError::PermissionError);
    }

    auto rows = members_.findMembersAfter(roomId, limit + 1, afterId);

    const bool hasMore = rows.size() > limit;
    if (hasMore)
        rows.pop_back();

    return Page<Member>{.elems = std::move(rows), .hasMore = hasMore};
}

std::vector<Room> ChatQueryService::searchRooms(const std::string &query, unsigned limit) const
{
    // Фильтр «только Public» уже вшит в репо (SELECT_BY_QUERY).
    return rooms_.findByQuery(query, limit);
}

} // namespace chat

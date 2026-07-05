#include <AuthService/UserQueryService.hpp>

#include <utility>

namespace auth
{

namespace
{

// auth::User -> UserFullView: отбрасывает passwordHash. Это и есть тот
// контракт безопасности на уровне типа, ради которого вводился QueryService.
UserFullView toFullView(User u)
{
    return UserFullView{
        .id = u.id,
        .username = std::move(u.username),
        .displayName = std::move(u.displayName),
        .country = u.country,
        .birthDate = u.birthDate,
        .registerTime = u.registerTime};
}

UserPublicView toPublicView(User u)
{
    return UserPublicView{
        .id = u.id,
        .displayName = std::move(u.displayName)};
}

} // namespace

UserQueryService::UserQueryService(const UserRepository &repo)
    : repo_(repo)
{
}

std::optional<UserFullView> UserQueryService::getUser(utils::UserId id) const
{
    auto user = repo_.findUserById(id);
    if (!user)
        return std::nullopt;
    return toFullView(std::move(*user));
}

std::vector<UserPublicView> UserQueryService::getDisplayInfos(std::span<const utils::UserId> ids) const
{
    auto users = repo_.findByIds(ids);

    std::vector<UserPublicView> result;
    result.reserve(users.size());
    for (auto &&u : users)
        result.push_back(toPublicView(std::move(u)));
    return result;
}

std::vector<UserFullView> UserQueryService::searchUsers(const std::string &query, unsigned limit) const
{
    auto users = repo_.findUsersByQuery(query, limit);

    std::vector<UserFullView> result;
    result.reserve(users.size());
    for (auto &&u : users)
        result.push_back(toFullView(std::move(u)));
    return result;
}

} // namespace auth

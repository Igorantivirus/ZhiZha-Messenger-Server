#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include <Utils/Types.hpp>

#include "Subservices/UserRepository.hpp"
#include "Types/UserFullView.hpp"
#include "Types/UserPublicView.hpp"

namespace auth
{

class UserQueryService
{
public:
    explicit UserQueryService(const UserRepository &repo);

    std::optional<UserFullView> getUser(utils::UserId id) const;
    std::vector<UserPublicView> getDisplayInfos(std::span<const utils::UserId> ids) const;
    std::vector<UserFullView> searchUsers(const std::string &query, unsigned limit) const;

private:
    const UserRepository &repo_;
};

} // namespace auth

#include <ChatService/Subservices/RoomPermissions.hpp>

namespace chat
{

namespace
{

bool isOwnerOrAdmin(utils::MemberRole r)
{
    return r == utils::MemberRole::Owner || r == utils::MemberRole::Admin;
}

} // namespace

bool RoomPermissions::canUpdateRoom(utils::MemberRole requester)
{
    return isOwnerOrAdmin(requester);
}

bool RoomPermissions::canDeleteRoom(utils::MemberRole requester)
{
    return requester == utils::MemberRole::Owner;
}

bool RoomPermissions::canInvite(utils::MemberRole requester, utils::JoinPolicy policy)
{
    switch (policy)
    {
    case utils::JoinPolicy::Closed:   return false;
    case utils::JoinPolicy::ByAdmin:  return isOwnerOrAdmin(requester);
    case utils::JoinPolicy::ByMember: return true;
    case utils::JoinPolicy::Public:   return true;
    }
    return false;
}

bool RoomPermissions::canKick(utils::MemberRole requester, utils::MemberRole target)
{
    if (target == utils::MemberRole::Owner)
        return false;
    if (requester == utils::MemberRole::Owner)
        return true;
    if (requester == utils::MemberRole::Admin)
        return true; // target здесь только Admin или Member — Owner отсечён выше
    return false;
}

bool RoomPermissions::canJoin(utils::JoinPolicy policy)
{
    return policy == utils::JoinPolicy::Public;
}

bool RoomPermissions::canLeave(utils::MemberRole /*requester*/)
{
    return true;
}

bool RoomPermissions::canSendMessage(utils::MemberRole requester, utils::WritePolicy policy)
{
    switch (policy)
    {
    case utils::WritePolicy::Everyone:   return true;
    case utils::WritePolicy::AdminsOnly: return isOwnerOrAdmin(requester);
    }
    return false;
}

} // namespace chat

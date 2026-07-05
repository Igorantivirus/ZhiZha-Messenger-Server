#pragma once

#include <Utils/Types.hpp>

namespace chat
{

// Чистые правила доступа комнат. Никакой БД — только роли и policy на входе,
// bool на выходе. Правила собраны здесь, чтобы ChatService не размазывал
// по своим методам ветвления по MemberRole/JoinPolicy/WritePolicy.
//
// Все методы static: класс — namespace-контейнер, состояния не имеет.
class RoomPermissions
{
public:
    // Изменение настроек комнаты (name, kind, joinPolicy, writePolicy).
    // Разрешено Owner и Admin.
    static bool canUpdateRoom(utils::MemberRole requester);

    // Удаление комнаты. Только Owner.
    static bool canDeleteRoom(utils::MemberRole requester);

    // Пригласить кого-то в комнату. Правила по joinPolicy:
    //   Closed   -> никто (комната закрытая наглухо)
    //   ByAdmin  -> только Owner/Admin
    //   ByMember -> любой участник комнаты
    //   Public   -> любой участник комнаты (то же, что ByMember;
    //               свобода входа снаружи не влияет на invite изнутри)
    static bool canInvite(utils::MemberRole requester, utils::JoinPolicy policy);

    // Кикнуть target из комнаты.
    //   Owner-а кикнуть НЕЛЬЗЯ никогда (уходит только сам через leave).
    //   Owner может кикнуть любого (Admin, Member).
    //   Admin может кикнуть Admin и Member.
    //   Member не может кикать никого.
    // Case «сам себя» этот метод не проверяет — это отдельная валидация
    // на входе (kickFromRoom vs leaveFromRoom — разные операции).
    static bool canKick(utils::MemberRole requester, utils::MemberRole target);

    // Присоединиться самому. Только если joinPolicy = Public.
    static bool canJoin(utils::JoinPolicy policy);

    // Уйти самому. Разрешено всем ролям, включая Owner
    // (Owner в системе только один, но может выйти).
    static bool canLeave(utils::MemberRole requester);

    // Написать сообщение в комнату.
    //   Everyone   -> любой участник
    //   AdminsOnly -> только Owner/Admin
    static bool canSendMessage(utils::MemberRole requester, utils::WritePolicy policy);
};

} // namespace chat

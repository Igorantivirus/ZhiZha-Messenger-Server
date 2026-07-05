# Протокол сервера

Протокол делится на две части: **HTTP** (REST-endpoint'ы для команд и чтений)
и **WebSocket** (единый канал уведомлений сервер → клиент + отправка сообщений
клиент → сервер).

Инициатор действия **всегда** попадает в получатели WS-события — у него могут
быть открыты другие устройства, им нужен sync. Само устройство, с которого
пришёл запрос, узнаёт результат:
- через HTTP-ответ (для REST-команд);
- через `MessageAckEvent` (для отправки сообщения по WS).

---

# HTTP

Каждый endpoint описан отдельной структурой `Operation` в
`include/Protocol/Operations`:

```cpp
#include <Protocol/Common/Operations.hpp>

struct Operation
{
    static constexpr HttpMethod method = HttpMethod::...;
    static constexpr Path path = API "...";
    struct PathParams { ... };
    struct QueryParams { ... };
    using Request = ...;   // Dto или void
    using Response = ...;  // Dto или void
};
```

- Тело запроса и ответа — JSON.
- Аутентификация — заголовок `Authorization: Bearer <accessToken>` (кроме
  `/api/*` и `/api/v1/auth/register`, `/login`, `/refresh`).
- Ошибки — единый `ErrorResponseDto { code, message }` + HTTP-статус.

## api: /api/v1/

Публичные endpoint'ы без авторизации.

| #   | Метод | Путь             | Что делает                                        | Operation                    | WS-рассылка |
| --- | ----- | ---------------- | ------------------------------------------------- | ---------------------------- | ----------- |
| 1   | GET   | `/api/v1/health` | Состояние сервера (пинг для healthcheck)          | `api::ServerHealthOperation` | —           |
| 2   | GET   | `/api/v1/info`   | Информация о сервере: версии, лимиты, TTL токенов | `api::ServerInfoOperation`   | —           |

## auth: /api/v1/auth

Регистрация, вход, управление токенами. Пары `access` (короткий, in-memory)
+ `refresh` (длинный, SQLite, одноразовый — при использовании ротируется).

| #   | Метод | Путь                      | Что делает                                                    | Operation                  | WS-рассылка |
| --- | ----- | ------------------------- | ------------------------------------------------------------- | -------------------------- | ----------- |
| 1   | POST  | `/api/v1/auth/register`   | Создать пользователя, вернуть пару токенов                    | `auth::RegisterOperation`  | —           |
| 2   | POST  | `/api/v1/auth/login`      | Проверить пароль, вернуть пару токенов                        | `auth::LoginOperation`     | —           |
| 3   | POST  | `/api/v1/auth/refresh`    | Обменять старый refresh на новую пару (старый инвалидируется) | `auth::RefreshOperation`   | —           |
| 4   | POST  | `/api/v1/auth/logout`     | Убить только этот refresh (access истечёт сам)                | `auth::LogoutOperation`    | —           |
| 5   | POST  | `/api/v1/auth/logout-all` | Убить все refresh пользователя (все устройства)               | `auth::LogoutAllOperation` | —           |

## rooms: /api/v1/rooms

Управление комнатами и участниками. Все команды-мутации возвращают
`Notifiables` внутри сервиса → транспорт рассылает соответствующие WS-события.

| #   | Метод  | Путь                                  | Что делает                                                                                       | Operation                      | WS-рассылка                                                                                |
| --- | ------ | ------------------------------------- | ------------------------------------------------------------------------------------------------ | ------------------------------ | ------------------------------------------------------------------------------------------ |
| 1   | GET    | `/api/v1/rooms`                       | Список комнат пользователя (страница `after` от `roomId`)                                        | `rooms::ListRoomsOperation`    | —                                                                                          |
| 2   | POST   | `/api/v1/rooms`                       | Создать комнату (creator = Owner, invitedUsers → Member)                                         | `rooms::CreateRoomOperation`   | `RoomCreatedEvent` → приглашённые + creator                                                |
| 3   | POST   | `/api/v1/rooms/loop`                  | Поиск публичных комнат (`joinPolicy = Public`) по подстроке имени                                | `rooms::LoopRoomsOperation`    | —                                                                                          |
| 4   | GET    | `/api/v1/rooms/<uint>`                | Карточка комнаты (только для участника)                                                          | `rooms::GetRoomOperation`      | —                                                                                          |
| 5   | DELETE | `/api/v1/rooms/<uint>`                | Удалить комнату (только Owner). Каскадно чистит messages и roomMembers                           | `rooms::DeleteRoomOperation`   | `RoomDeletedEvent` → все бывшие участники (включая Owner)                                  |
| 6   | PUT    | `/api/v1/rooms/<uint>`                | Обновить настройки комнаты (Owner + Admin). Смена типа на Direct — только если ровно 2 участника | `rooms::UpdateRoomOperation`   | `RoomUpdatedEvent` → все участники                                                         |
| 7   | GET    | `/api/v1/rooms/<uint>/members`        | Страница участников комнаты (`after` от `userId`)                                                | `rooms::ListMembersOperation`  | —                                                                                          |
| 8   | POST   | `/api/v1/rooms/<uint>/members`        | Пригласить пользователя (правила по `joinPolicy`)                                                | `rooms::InviteMemberOperation` | `UserJoinEvent { roomId, invitedId }` → все участники (включая приглашённого и инициатора) |
| 9   | POST   | `/api/v1/rooms/<uint>/members/me`     | Самостоятельно войти (только если `joinPolicy = Public`)                                         | `rooms::JoinRoomOperation`     | `UserJoinEvent { roomId, self }` → все участники                                           |
| 10  | DELETE | `/api/v1/rooms/<uint>/members/me`     | Самостоятельно выйти                                                                             | `rooms::LeaveRoomOperation`    | `UserLeftEvent { roomId, self }` → все бывшие участники (включая уходящего)                |
| 11  | DELETE | `/api/v1/rooms/<uint>/members/<uint>` | Выгнать участника. Owner неприкосновенен, Admin → Admin/Member можно, Member — не может кикать   | `rooms::KickMemberOperation`   | `UserLeftEvent { roomId, kicked }` → все бывшие участники (включая кикнутого и инициатора) |

## users: /api/v1/users

Профили пользователей. Read через `UserQueryService` (view-типы без
`passwordHash`), write — через `AuthService`.

| #   | Метод | Путь                   | Что делает                                               | Operation                   | WS-рассылка                                                                        |
| --- | ----- | ---------------------- | -------------------------------------------------------- | --------------------------- | ---------------------------------------------------------------------------------- |
| 1   | GET   | `/api/v1/users/loop`   | Поиск пользователей по подстроке `username`              | `users::LoopUsersOperation` | —                                                                                  |
| 2   | PUT   | `/api/v1/users/me`     | Изменить свои редактируемые поля профиля (с валидациями) | `users::ChangeMeOperation`  | `UserUpdateEvent` → все, кто состоит в общих комнатах *(TODO: пока не подключено)* |
| 3   | GET   | `/api/v1/users/me`     | Получить свой профиль                                    | `users::GetMeOperation`     | —                                                                                  |
| 4   | GET   | `/api/v1/users/<uint>` | Получить публичный профиль пользователя                  | `users::GetUserOperation`   | —                                                                                  |

## messages: /api/v1/rooms/*/messages

Чтение истории сообщений комнаты. Отправка сообщения — через WS, не HTTP.

| #   | Метод | Путь                            | Что делает                                                                                                        | Operation                        | WS-рассылка |
| --- | ----- | ------------------------------- | ----------------------------------------------------------------------------------------------------------------- | -------------------------------- | ----------- |
| 1   | GET   | `/api/v1/rooms/<uint>/messages` | Страница сообщений комнаты (`after` или `before` — ровно один режим), опционально с отправителями (`withSenders`) | `messages::GetMessagesOperation` | —           |

---

# WebSocket

Единый endpoint: **`/ws`** — авторизация через тот же `Bearer`-токен на этапе
`onaccept`. Одно WS-соединение = одно устройство; один пользователь может
иметь несколько активных соединений одновременно.

Все сообщения — JSON с полем `type` (enum `WsEventType`), по нему делается
диспатч. События определены в `include/Protocol/Ws/Events.hpp`.

## Универсальные

| Событие                     | Направление     | Что значит                                        |
| --------------------------- | --------------- | ------------------------------------------------- |
| `ErrorEvent { error, msg }` | Сервер → клиент | Ошибка обработки предыдущего WS-сообщения клиента |
| `PingEvent`                 | Обе стороны     | Keep-alive, запрос отклика                        |
| `PongEvent`                 | Обе стороны     | Keep-alive, ответ на ping                         |

## Клиент → сервер

| Событие                                                       | Что делает                                           | Что сервер шлёт в ответ                                                                                                                                                                                   |
| ------------------------------------------------------------- | ---------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `DispatcinghNewMessageEvent { roomId, localMessageId, text }` | Сохранить сообщение в комнате и разослать участникам | Тому же соединению — `MessageAckEvent { localMessageId → globalMessageId, roomId, createAt }`. Всем **другим** соединениям участников (включая другие устройства отправителя) — `SendingNewMessageEvent`. |

Ack-паттерн нужен, чтобы клиент связал своё локальное сообщение
(`localMessageId`) с назначенным сервером `globalMessageId`.

## Сервер → клиент

События этой группы сервер отправляет реактивно — либо в ответ на действие
самого клиента (по WS), либо как результат HTTP-команды другого пользователя.
Столбец «Триггер» — что именно приводит к рассылке.

| Событие                                                                   | Триггер                                                                                       | Кому отправляется                                                                                    |
| ------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| `SendingNewMessageEvent { roomId, messageId, senderId, text, createdAt }` | WS `DispatcinghNewMessageEvent`                                                               | Всем соединениям участников комнаты, **кроме** соединения-отправителя (ему уходит `MessageAckEvent`) |
| `MessageAckEvent { localMessageId, globalMessageId, roomId, createAt }`   | WS `DispatcinghNewMessageEvent`                                                               | Только тому соединению, с которого пришло сообщение                                                  |
| `RoomCreatedEvent { room }`                                               | HTTP `POST /rooms`                                                                            | Приглашённым + creator                                                                               |
| `RoomDeletedEvent { roomId }`                                             | HTTP `DELETE /rooms/{id}`                                                                     | Всем бывшим участникам комнаты                                                                       |
| `RoomUpdatedEvent { room }`                                               | HTTP `PUT /rooms/{id}`                                                                        | Всем участникам комнаты                                                                              |
| `UserJoinEvent { roomId, userId }`                                        | HTTP `POST /rooms/{id}/members` (invite) или `POST /rooms/{id}/members/me` (join)             | Всем участникам комнаты, включая только что вошедшего                                                |
| `UserLeftEvent { roomId, userId }`                                        | HTTP `DELETE /rooms/{id}/members/me` (leave) или `DELETE /rooms/{id}/members/{userId}` (kick) | Всем бывшим участникам комнаты, включая того, кто ушёл/кого выгнали                                  |
| `UserUpdateEvent { userId, info }`                                        | HTTP `PUT /users/me`                                                                          | Всем пользователям, состоящим в общих комнатах с ним *(TODO: пока не подключено)*                    |

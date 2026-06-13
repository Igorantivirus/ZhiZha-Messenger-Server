# ZhiZha Server — API, оценка и дорожная карта

> Состояние на 2026-06-03. Версия сервера `0.1.0` (in development).
> Анализ кода в `src/`. Документ описывает текущий API, оценивает сервер
> как чат-сервер и предлагает дорожную карту развития.

---

## 1. Архитектура (кратко)

Сервер построен аккуратно и по слоям — это сильная сторона проекта:

```
ServerApplication (композиционный корень, DI вручную)
├── Transport
│   ├── HttpServer ── ApiController / AuthController / UsersController / RoomsController / MessagesController
│   └── WsServer   ── /ws (auth в handshake, диспетчеризация по type)
├── Доменные сервисы
│   ├── AuthService   (register / login / refresh / logout)
│   └── ChatService   (rooms / messages / members / рассылка через WS)
├── Sessions/SessionManager (реестр живых WS-соединений, потокобезопасен)
└── Хранилища (SQLite + интерфейсы)
    ├── User / RefreshToken
    └── Room / Message / RoomMembers
```

Транспорт — [Crow](extern/crow/crow.h), сериализация — `nlohmann::json`,
enum'ы конвертируются строками через `magic_enum`
([Protocol/Parsing.hpp](../src/Protocol/Parsing.hpp)). Все DTO протокола
изолированы в `protocol::*` и не зависят от транспорта — это правильно.

**Что хорошо реализовано:**
- Чистое разделение слоёв, DI через конструкторы, интерфейсы для хранилищ.
- Единый машинно-читаемый `ErrorCode` для HTTP и WS ([ErrorCode.hpp](../src/Protocol/ErrorCode.hpp)).
- Rotating refresh-токены (одноразовые), отдельный access-store ([DummyTokenService.hpp](../src/Auth/Impl/DummyTokenService.hpp)).
- Timing-safe login (холостая проверка хеша при отсутствии юзера, [AuthService.hpp:49](../src/Auth/AuthService.hpp#L49)).
- Потокобезопасный `SessionManager` с мульти-девайс поддержкой (userId → N соединений).
- Курсорная пагинация сообщений (`beforeId` / `afterId` / `limit`) с `hasMore`.
- Серверное время для `createdAt` (не доверяем клиенту).

---

## 2. Справочник API

### 2.1 HTTP REST (`/api/v1`)

Аутентификация на защищённых путях — заголовок `Authorization: Bearer <accessToken>`.
Тело запросов/ответов — JSON. Ошибки имеют единый формат `ErrorResponse`.

#### Общие структуры

**`ErrorResponse`** ([Api.hpp](../src/Protocol/Api.hpp))
```jsonc
{ "code": "InvalidFormat", "message": "..." }   // code — строковый ErrorCode
```

---

#### Service / Discovery — `ApiController`

| Метод | Путь             | Auth | Запрос | Ответ             |
| ----- | ---------------- | ---- | ------ | ----------------- |
| GET   | `/api/v1/health` | —    | —      | `{"status":"ok"}` |
| GET   | `/api/v1/info`   | —    | —      | `InfoResponse`    |

**`InfoResponse`**
```jsonc
{ "serverName": "ZhiZha", "version": "0.1.0", "wsEndpoint": "/ws",
  "accessTtl": 900, "refreshTtl": 7200 }
```

---

#### Auth — `AuthController`

| Метод | Путь                      | Auth   | Запрос            | Ответ                     |
| ----- | ------------------------- | ------ | ----------------- | ------------------------- |
| POST  | `/api/v1/auth/register`   | —      | `RegisterRequest` | 201 `AuthSuccessResponse` |
| POST  | `/api/v1/auth/login`      | —      | `LoginRequest`    | 200 `AuthSuccessResponse` |
| POST  | `/api/v1/auth/refresh`    | —      | `RefreshRequest`  | 200 `AuthSuccessResponse` |
| POST  | `/api/v1/auth/logout`     | —      | `LogoutRequest`   | 204                       |
| POST  | `/api/v1/auth/logout-all` | Bearer | —                 | 204                       |

```jsonc
// RegisterRequest
{ "username": "...", "password": "...", "displayName": "..." }
// LoginRequest
{ "username": "...", "password": "..." }
// RefreshRequest / LogoutRequest
{ "refreshToken": "..." }
// AuthSuccessResponse
{ "userId": 1, "accessToken": "...", "refreshToken": "...",
  "accessExpiresIn": 900, "refreshExpiresIn": 7200 }
```

---

#### Users — `UsersController`

| Метод | Путь                               | Auth   | Запрос       | Ответ                        |
| ----- | ---------------------------------- | ------ | ------------ | ---------------------------- |
| GET   | `/api/v1/me`                       | Bearer | —            | `MeResponse`                 |
| GET   | `/api/v1/users/loop?query=&limit=` | Bearer | query params | `UsersLoopByExampleResponse` |
| GET   | `/api/v1/users/<uint>`             | Bearer | —            | `UserResponse`               |

```jsonc
// MeResponse
{ "userId": 1, "username": "...", "displayname": "...", "registerTime": 0 }
// UserResponse
{ "userId": 1, "username": "...", "displayname": "..." }
// UsersLoopByExampleResponse  (поиск по подстроке)
{ "users": { "1": { "username": "...", "displayname": "..." } } }
```
> ⚠️ Путь `/users/loop` назван неудачно (это поиск по примеру-запросу,
> а не «loop»). Лучше `/api/v1/users/search` или `/api/v1/users?query=`.

---

#### Rooms — `RoomsController`

| Метод  | Путь                                     | Auth   | Запрос                | Ответ                      |
| ------ | ---------------------------------------- | ------ | --------------------- | -------------------------- |
| GET    | `/api/v1/rooms?lastLoadedRoomId=&limit=` | Bearer | query                 | `GetRoomsResponse`         |
| POST   | `/api/v1/rooms`                          | Bearer | `CreateRoomRequest`   | 200 `CreateRoomResponse`   |
| GET    | `/api/v1/rooms/<uint>`                   | Bearer | —                     | `RoomInfoResponse`         |
| GET    | `/api/v1/rooms/<uint>/members`           | Bearer | —                     | `RoomsMembersInfoResponse` |
| POST   | `/api/v1/rooms/<uint>/members`           | Bearer | `InviteMemberRequest` | 204                        |
| DELETE | `/api/v1/rooms/<uint>/members/me`        | Bearer | —                     | 204                        |

```jsonc
// RoomInfo (вложенная в Room / CreateRoomRequest)
{ "kind": "Direct|Group|Channel",
  "joinPolicy": "Closed|ByMember|ByAdmin|Public",
  "writePolicy": "Everyone|AdminsOnly" }
// Room
{ "id": 1, "name": "...", "info": { ...RoomInfo } }
// CreateRoomRequest
{ "roomName": "...", "roomInfo": { ...RoomInfo }, "invitedUsers": [2,3] }
// CreateRoomResponse
{ "roomId": 1 }
// GetRoomsResponse
{ "rooms": [ { "roomInfo": { ...Room }, "lastMessage": { ...Message } } ],
  "postMessageSenders": { "2": { "username": "...", "displayname": "..." } },
  "hasMore": false }
// RoomInfoResponse
{ "room": { ...Room } }
// RoomsMembersInfoResponse
{ "members": [ { "userId": 2 } ] }
// InviteMemberRequest
{ "invitedId": 2 }
```

---

#### Messages — `MessagesController`

| Метод | Путь                                                      | Auth   | Запрос | Ответ              |
| ----- | --------------------------------------------------------- | ------ | ------ | ------------------ |
| GET   | `/api/v1/rooms/<uint>/messages?beforeId=&afterId=&limit=` | Bearer | query  | `MessagesResponse` |

```jsonc
// Message
{ "id": 1, "fromUserId": 2, "text": "...", "createdAt": 0 }
// MessagesResponse
{ "roomId": 1, "messages": [ { ...Message } ], "hasMore": false }
```
- `beforeId` — скролл вверх (старше курсора). `afterId` — догрузка новых.
- Взаимоисключающие. `limit` по умолчанию 50, максимум 100.
- `mark-read` (`POST /rooms/<uint>/read`) — **закомментирован**, не реализован.

---

### 2.2 WebSocket (`/ws`)

Аутентификация — в handshake (`Authorization: Bearer <accessToken>`).
Невалидный токен → upgrade отклоняется (HTTP 400). Конверт сообщения
дискриминируется по полю `type` (строка имени enum).

**Клиент → Сервер**

| `type`        | Структура                                            | Реализовано в `WsServer::dispatch` |
| ------------- | ---------------------------------------------------- | ---------------------------------- |
| `Ping`        | `{ "type": "Ping" }`                                 | ✅ → `Pong`                         |
| `SendMessage` | `{ "type":"SendMessage", "roomId":1, "text":"..." }` | ✅                                  |
| `CreateRoom`  | `{ "type":"CreateRoom", ... }`                       | ❌ (есть DTO, нет ветки)            |
| `LeaveRoom`   | `{ "type":"LeaveRoom", "roomId":1 }`                 | ❌ (есть DTO, нет ветки)            |

**Сервер → Клиент**

| `type`        | Структура                                                                         | Кто шлёт        |
| ------------- | --------------------------------------------------------------------------------- | --------------- |
| `Pong`        | `{ "type":"Pong" }`                                                               | ответ на Ping   |
| `Error`       | `{ "type":"Error", "code":"...", "message":"..." }`                               | при ошибке      |
| `NewMessage`  | `{ "type":"NewMessage", "messageId", "roomId", "senderId", "text", "createdAt" }` | `onSendMessage` |
| `UserLeft`    | `{ "type":"UserLeft", "roomId", "userId" }`                                       | `leaveRoom`     |
| `RoomCreated` | `{ "type":"RoomCreated", "roomId" }`                                              | `createRoom`    |

---

## 3. Оценка удобства как чат-сервера

### 3.1 Сильные стороны
- Понятный, предсказуемый REST + событийный WS. Хорошее разделение
  «команды через REST / события через WS».
- Курсорная пагинация и `hasMore` — клиент может бесконечно скроллить.
- Единый `ErrorCode` упрощает клиент.
- Мульти-девайс из коробки (рассылка во все сессии пользователя).
- Заготовка под read-receipts уже есть в схеме БД (`lastReadMessageId`).

### 3.2 Критичные проблемы и несоответствия

**Безопасность / корректность (исправить в первую очередь):**

1. **`DummyTokenService` / `DummyPasswordHasher`** — заглушки. Токены —
   случайные строки в in-memory store; пароли, судя по имени, не хешируются
   нормально. **Нельзя в продакшен.** Нужны bcrypt/argon2 и JWT или
   защищённые opaque-токены с TTL-очисткой.

2. **Access-токены в памяти** ([DummyAccessTokenStore]) — теряются при
   рестарте, не масштабируются на несколько инстансов. После рестарта все
   WS-сессии «протухают».

3. **`getRoomsByUser` — N+1 запросов.** Для каждой комнаты отдельный
   `findLastMessageInRoom` ([ChatService.hpp:211](../src/ChatService/ChatService.hpp#L211)).
   Плюс в `handleListRooms` ещё цикл `findUserById` на каждого отправителя.

4. **`handleListRooms` не валидирует `limit`** — `std::atoi(nullptr)` UB,
   `limit + 1` без проверки знака/диапазона
   ([RoomsController.hpp:50-58](../src/Transport/Controllers/RoomsController.hpp#L50-L58)).
   `intLimit` сравнивается с `rooms->size()` (signed/unsigned mismatch).

5. **`getMessages` игнорирует сконфигурированный `maxMessageSize`** —
   в `onSendMessage` захардкожен лимит `1000` вместо `maxMessageSize_`
   ([ChatService.hpp:57](../src/ChatService/ChatService.hpp#L57)).

6. **Передача владения комнатой не реализована** — TODO в `leaveRoom`:
   `updateRole` закомментирован, owner уходит, но новый owner не назначается
   ([ChatService.hpp:294](../src/ChatService/ChatService.hpp#L294)).

7. **`onUserConnected/Disconnected` пустые** — presence не реализован,
   хотя `SessionManager::isOnline` уже есть.

**Функциональные пробелы как у чат-сервера:**

8. **Нельзя редактировать/удалять сообщения.** Нет `PUT/DELETE /messages/<id>`,
   нет событий `MessageEdited` / `MessageDeleted`.

9. **Нет индикатора «печатает» (typing), нет presence-событий** (online/offline),
   нет delivery/read receipts по WS (хотя `updateLastRead` в репозитории есть).

10. **Нет вложений/медиа** — только текст. Нет загрузки файлов.

11. **WS `CreateRoom` / `LeaveRoom` объявлены, но не обрабатываются** —
    клиент может их послать и получить `UnknownMessageType`.

12. **Нет ack/доставки сообщений отправителю** — `onSendMessage` возвращает
    `MessageId` в `void(...)`, клиент по WS не узнаёт id своего сообщения
    ([WsServer.hpp:150](../src/Transport/WsServer.hpp#L150)).

13. **Нет управления участниками** — нельзя кикнуть, сменить роль,
    забанить. `updateRole` отсутствует в репозитории.

14. **Нет rate-limiting / антифлуда**, нет ограничения размера WS-кадра
    на уровне приложения.

15. **`std::cout` логирование** в WsServer — нет структурированного логгера.

16. **Нет CORS / TLS** конфигурации на уровне приложения (для web-клиента).

17. **Нет миграций БД** — схема создаётся `CREATE TABLE IF NOT EXISTS`,
    эволюция схемы не поддержана.

---

## 4. Дорожная карта

### Этап 0 — «Сделать безопасным» (блокеры продакшена)
- [ ] Заменить `DummyPasswordHasher` на argon2id / bcrypt.
- [ ] Заменить `DummyTokenService`: JWT (access) + персистентный refresh,
      либо opaque-токены с фоновой очисткой просроченных.
- [ ] Персистентность access-store (или stateless JWT) — пережить рестарт.
- [ ] Применить `maxMessageSize_` из конфига вместо хардкода `1000`.
- [ ] Починить валидацию `limit` в `handleListRooms` (atoi(nullptr) UB,
      signed/unsigned).
- [ ] Структурированный логгер вместо `std::cout`.

### Этап 1 — «Корректный чат» (функциональная полнота MVP)
- [ ] Доделать `leaveRoom`: передача владения (`updateRole`) при уходе owner.
- [ ] Реализовать WS-ветки `CreateRoom` и `LeaveRoom` (или убрать из enum).
- [ ] Ack отправителю по WS: вернуть `messageId` + `createdAt` отправившему.
- [ ] Реализовать read-receipts: `POST /rooms/<id>/read` (раскомментировать
      `handleMarkRead`) + WS-событие `MessageRead`.
- [ ] Редактирование/удаление сообщений: REST + WS `MessageEdited`/`MessageDeleted`.
- [ ] Управление участниками: kick, change-role (нужен `updateRole` в репо).
- [ ] Переименовать `/users/loop` → `/users/search`.

### Этап 2 — «Живой чат» (UX-фичи)
- [ ] Presence: заполнить `onUserConnected/Disconnected`, рассылать
      `UserOnline`/`UserOffline` участникам общих комнат.
- [ ] Typing-индикаторы (WS, эфемерные, без записи в БД).
- [ ] Delivery receipts (доставлено/прочитано) поверх `lastReadMessageId`.
- [ ] Непрочитанные: счётчик unread на комнату в `GetRoomsResponse`.
- [ ] Вложения: загрузка файлов (S3/локально) + сообщения с медиа.

### Этап 3 — «Масштаб и эксплуатация»
- [ ] Устранить N+1 в `getRoomsByUser` (JOIN / batch-запрос последних
      сообщений и отправителей).
- [ ] Rate-limiting и антифлуд (per-user / per-IP).
- [ ] Миграции схемы БД (версионирование, отдельный модуль).
- [ ] TLS + CORS конфигурация; reverse-proxy-friendly заголовки.
- [ ] Метрики/health с реальными проверками БД и пулом соединений.
- [ ] Pub/Sub слой (Redis) для горизонтального масштабирования WS
      (несколько инстансов сервера видят сессии друг друга).
- [ ] Heartbeat/timeout для мёртвых WS-соединений (server-side ping).

### Этап 4 — «Зрелость»
- [ ] Поиск по сообщениям (FTS5 в SQLite или переход на Postgres).
- [ ] Реакции на сообщения, ответы (reply-to), пересылка.
- [ ] Пуш-уведомления (offline-доставка).
- [ ] Push для оффлайн-устройств, дедупликация событий по `clientMessageId`
      (идемпотентная отправка — клиент шлёт uuid, сервер дедупит).
- [ ] OpenAPI-спека и автотесты транспорта.

---

## 5. Быстрые победы (можно сделать сразу)

| Что                               | Где                                                                                  | Эффект                             |
| --------------------------------- | ------------------------------------------------------------------------------------ | ---------------------------------- |
| `maxMessageSize_` вместо `1000`   | [ChatService.hpp:57](../src/ChatService/ChatService.hpp#L57)                         | конфиг начинает работать           |
| Валидация `limit` (atoi nullptr)  | [RoomsController.hpp:50](../src/Transport/Controllers/RoomsController.hpp#L50)       | убрать UB/краш                     |
| Ack `messageId` отправителю по WS | [WsServer.hpp:150](../src/Transport/WsServer.hpp#L150)                               | клиент знает id                    |
| Раскомментировать `mark-read`     | [MessagesController.hpp:86](../src/Transport/Controllers/MessagesController.hpp#L86) | read-receipts                      |
| Реализовать `updateRole`          | репозиторий членов                                                                   | передача владения, роли            |
| WS `CreateRoom`/`LeaveRoom` ветки | [WsServer.hpp:131](../src/Transport/WsServer.hpp#L131)                               | убрать ложный `UnknownMessageType` |

# Messenger2 Server JSON API

Актуально для текущей реализации сервера (Crow WebSocket). Все сообщения передаются **текстом** (UTF-8) как JSON-объекты. **Binary WebSocket frames не поддерживаются**.

## Точки входа

### HTTP `GET /info`
Возвращает JSON:
```json
{
  "alive": true,
  "server-name": "Messenger2 Server"
}
```

### WebSocket `GET /ws`
После открытия WebSocket сервер отправляет стартовое сообщение `hello` и ожидает регистрацию.

## Общие правила

- Каждое клиентское сообщение обязано иметь поле `"type"` (string).
- Все `id` и числовые данные в JSON передаются **числами**, не строками.
- Если клиент не зарегистрировался за `registration-timeout-seconds`, соединение будет закрыто сервером.
- До регистрации разрешён только `type = "register"`. Все остальные типы до регистрации вернут ошибку `not-authorized`.

## Сообщения: Server -> Client

### `hello`
Сценарий: отправляется сразу после подключения к WebSocket, до регистрации.

```json
{
  "type": "hello",
  "authorized": false,
  "registration-timeout-seconds": 20,
  "server-name": "Messenger2 Server"
}
```

Поля:
- `type`: `"hello"`.
- `authorized`: `false` до регистрации.
- `registration-timeout-seconds`: сколько секунд даётся на регистрацию.
- `server-name`: имя сервера.

### Registration result (без `type`)
Сценарий: ответ на успешную регистрацию.

```json
{
  "registered": true,
  "user-id": 1,
  "server-public-key": "server-public-key-stub",
  "users-chats": [1],
  "server-name": "Messenger2 Server",
  "protocol-version": "1.0"
}
```

Поля:
- `registered`: `true` если регистрация прошла.
- `user-id`: числовой ID пользователя, который клиент обязан использовать дальше.
- `server-public-key`: публичный ключ сервера (пока заглушка).
- `users-chats`: список ID комнат (чатов), где пользователь состоит.
- `server-name`: имя сервера.
- `protocol-version`: версия протокола.

### `chat-msg` (broadcast)
Сценарий: сервер рассылает сообщение всем участникам комнаты.

```json
{
  "type": "chat-msg",
  "user-id": 1,
  "chat-id": 1,
  "message": "hi",
  "server-message-id": 1
}
```

Поля:
- `type`: `"chat-msg"`.
- `user-id`: кто отправил.
- `chat-id`: в какую комнату.
- `message`: текст сообщения.
- `server-message-id`: монотонный серверный ID сообщения.

### `room-created`
Сценарий: ответ инициатору на создание комнаты.

```json
{
  "type": "room-created",
  "created": true,
  "chat-id": 2,
  "participant-user-ids": [1, 5, 7]
}
```

Поля:
- `type`: `"room-created"`.
- `created`: `true` если комната создана.
- `chat-id`: ID новой комнаты.
- `participant-user-ids`: итоговый список добавленных пользователей.

Примечания:
- ID комнаты генерируется сервером; сейчас комнаты создаются начиная с `2`.
- В комнату добавляются только те пользователи из списка, которые **сейчас подключены и зарегистрированы**; остальные ID игнорируются.

### `room-left`
Сценарий: ответ пользователю на выход из комнаты.

```json
{
  "type": "room-left",
  "left": true,
  "user-id": 1,
  "chat-id": 2
}
```

Поля:
- `type`: `"room-left"`.
- `left`: `true` если выход выполнен.
- `user-id`: кто вышел.
- `chat-id`: из какой комнаты.

Примечание:
- Если после выхода комната (кроме `chat-id=1`) становится пустой, сервер удаляет её.

### `error`
Сценарий: любые ошибки валидации, протокола, доступа.

```json
{
  "type": "error",
  "code": "invalid-json",
  "message": "Field 'type' is required"
}
```

Поля:
- `type`: `"error"`.
- `code`: машинный код ошибки.
- `message`: описание.

Коды ошибок (текущие):
- `binary-not-supported`
- `invalid-json`
- `public-key-required`
- `not-authorized`
- `invalid-chat-payload`
- `wrong-user-id`
- `chat-access-denied`
- `chat-not-found`
- `invalid-create-room-payload`
- `invalid-leave-room-payload`
- `unknown-message-type`
- `already-registered`

## Сообщения: Client -> Server

### `register`
Сценарий: первая регистрация после `hello`. Если регистрация не отправлена за таймаут, соединение закрывается.

```json
{
  "type": "register",
  "public-key": "client-public-key-stub",
  "username": "alice",
  "client-version": "0.1.0"
}
```

Поля:
- `type`: `"register"`.
- `public-key`: строка с публичным ключом клиента (пока заглушка).
- `username`: опционально, отображаемое имя.
- `client-version`: опционально, версия клиента.

### `chat-msg`
Сценарий: отправка сообщения в чат (комнату).

```json
{
  "type": "chat-msg",
  "user-id": 1,
  "chat-id": 1,
  "message": "hi",
  "client-message-id": 123
}
```

Поля:
- `type`: `"chat-msg"`.
- `user-id`: ваш ID из ответа регистрации.
- `chat-id`: ID комнаты.
- `message`: текст сообщения.
- `client-message-id`: опционально, клиентский ID сообщения (0/отсутствие = не задано).

Примечания:
- Сервер проверяет, что `user-id` совпадает с `user-id`, выданным этому соединению.
- Сервер проверяет, что пользователь состоит в комнате `chat-id`.

### `create-room`
Сценарий: создание новой комнаты с участниками.

```json
{
  "type": "create-room",
  "user-id": 1,
  "participant-user-ids": [5, 7],
  "is-private": true
}
```

Поля:
- `type`: `"create-room"`.
- `user-id`: ваш ID из ответа регистрации.
- `participant-user-ids`: список ID пользователей, которых нужно добавить в комнату.
- `is-private`: опционально, `true` по умолчанию.

### `leave-room`
Сценарий: выход из комнаты по ID.

```json
{
  "type": "leave-room",
  "user-id": 1,
  "chat-id": 2
}
```

Поля:
- `type`: `"leave-room"`.
- `user-id`: ваш ID из ответа регистрации.
- `chat-id`: ID комнаты, из которой хотите выйти.

## Комнаты по умолчанию

- `chat-id = 1` существует всегда (публичная комната).
- После успешной регистрации пользователь автоматически добавляется в `chat-id = 1`.


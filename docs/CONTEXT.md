# Контекст работы над ZhiZha-Server (для Claude)

> Этот файл — перенос рабочей памяти, чтобы продолжать работу с Claude на другом
> устройстве. Скопируй разделы в `~/.claude/projects/<хэш-пути-проекта>/memory/`
> (по одному файлу на раздел + индекс MEMORY.md) или просто попроси Claude
> «перенеси контекст из docs/CONTEXT.md в память».

---

## working-process.md

```
---
name: working-process
description: Как пользователь хочет, чтобы я работал над сервером ZhiZha — обсуждение, утверждение плана, потом код
metadata:
  type: feedback
---

Пользователь ведёт разработку сервера ZhiZha и хочет работать так: он говорит, ЧТО хочет реализовать → мы ОБЯЗАТЕЛЬНО обсуждаем дизайн ради чистой архитектуры и расширяемости → я предлагаю план → он утверждает → только ПОСЛЕ утверждения я пишу реализацию.

**Why:** Пользователь ценит чистую архитектуру и сам принимает архитектурные решения; преждевременный код без обсуждения нежелателен.

**How to apply:** Перед написанием кода по новой фиче — проанализировать затронутые файлы, обсудить развилки (через AskUserQuestion для конкретных решений), дать рекомендацию, дождаться утверждения. Пользователь иногда сам дорабатывает код между сессиями — всегда перечитывать актуальное состояние файлов перед продолжением, не полагаться на прошлый черновик. См. [[code-conventions]], [[build-setup]].
```

---

## code-conventions.md

```
---
name: code-conventions
description: Конвенции кода ZhiZha-Server — именование, разделение слоёв, особое правило про протокол
metadata:
  type: feedback
---

Обязательные соглашения при написании кода для ZhiZha-Server:

1. **Именование:** camelCase для методов, переменных, полей, параметров. PascalCase для файлов, папок, классов, enum'ов и значений enum'ов.
2. **Разделение ответственности** между слоями строго соблюдается (транспорт не лезет в домен и наоборот; маппинг ошибок — единый источник истины, см. [[server-architecture]]).
3. **Протокол (`src/Protocol/`) нельзя редактировать без явного точного подтверждения пользователя.** Эта часть общая для сервера и клиента — изменения только после прямого «да, можно править протокол».

**Why:** Протокол — контракт между сервером и клиентом (отдельный репозиторий клиента), молчаливое изменение сломает совместимость. Чистое разделение слоёв — приоритет пользователя.

**How to apply:** Перед правкой любого файла в `src/Protocol/` (Ws.hpp, Api.hpp, Auth.hpp, Messages.hpp, Rooms.hpp, Users.hpp, Types.hpp, ErrorCode.hpp, Parsing.hpp) — спросить подтверждение. Маппинг доменных ошибок в код/текст держать в одном месте (HttpHelpers), а не дублировать по слоям.
```

---

## build-setup.md

```
---
name: build-setup
description: Сборка ZhiZha-Server — MSVC, пресет Win-BT, я НЕ собираю проект сам
metadata:
  type: feedback
---

Компилятор — MSVC. Пользователь использует CMake-пресет **Win-BT**. Библиотеки подключаются либо из исходников прямо в проект (nlohmann, crow), либо собираются install и подключаются как MT.

**Why:** Пользователь сам собирает и тестирует на своей машине; у него настроено окружение MSVC/MT.

**How to apply:** НЕ запускать сборку проекта напрямую (никаких cmake build / msbuild по своей инициативе) — пользователь собирает и тестирует сам. Свои локальные микротесты при надобности можно. Код должен быть совместим с MSVC (C++23: std::expected, std::ranges уже используются). См. [[working-process]].
```

---

## server-architecture.md

```
---
name: server-architecture
description: Архитектура ZhiZha-Server — слои, стек, ключевые файлы и паттерны
metadata:
  type: project
---

ZhiZha-Server — чат-сервер на C++ (header-only стиль, всё в `.hpp`). Слои:

- **App:** `ServerApplication` — композиционный корень, ручной DI через конструкторы. `ServerConfig`.
- **Transport:** `HttpServer` + контроллеры (`Api/Auth/Users/Rooms/Messages Controller`), `WsServer` (/ws). `HttpHelpers` — общие хелперы транспорта, в т.ч. единый маппинг `ChatError` → код/текст (`mapChatErrorInfo` приватный → `mapChatError` для HTTP, `mapChatErrorWs` для WS, возвращает `protocol::ws::ErrorMessage`).
- **Домен:** `AuthService`, `ChatService`. Возвращают `std::expected<T, Error>` / `std::optional<Error>`.
- **Sessions:** `SessionManager` (потокобезопасный реестр живых WS-соединений, мульти-девайс: userId → N соединений). Методы: `sendToUser`, `sendToConnection`, `sendToUserExcept(userId, exceptConn, payload)`.
- **Хранилища:** интерфейсы `I*Repository` + SQLite-реализации в `Impl/`.
- **Protocol:** DTO в `protocol::*`, изолированы от транспорта. Enum'ы ↔ строки через magic_enum (`Parsing.hpp`, макрос `NLOHMANN_JSON_MAGIC_ENUM`). WS-конверт дискриминируется по полю `type`; парсинг через `parseMessageFromClient/Server` → `std::variant`, диспетчеризация в WsServer через `std::visit` по типизированным перегрузкам `handle(conn, msg)`.

**Стек:** Crow (transport, `extern/crow/`), nlohmann::json (сериализация), magic_enum (enum↔string), SQLite (хранилище).

Состояние и дорожная карта подробно: `docs/API_AND_ROADMAP.md` (версия 0.1.0, in development). Известные пробелы: Dummy-заглушки для хеша паролей и токенов (не для прода), N+1 в getRoomsByUser, нет presence/typing/edit-delete/read-receipts/вложений. См. [[code-conventions]], [[build-setup]].
```

---

## feature-message-ack.md

```
---
name: feature-message-ack
description: Завершённая фича — WS-подтверждение отправителю сообщения (usersMessageId → глобальный messageId)
metadata:
  type: project
---

Реализовано (июнь 2026): подтверждение отправки сообщения по WebSocket. Клиент шлёт `SendMessage` с локальным `usersMessageId` (тип `MessageId`/uint64); после сохранения сервер шлёт ТОМУ ЖЕ соединению `MessageAck`, связывающий локальный id с присвоенным глобальным `messageId`. Локальный id нигде не хранится — только проксируется обратно в ack.

Итоговая реализация (пользователь дорабатывал поверх черновика):
- `protocol::ws::SendMessageRequest` + поле `MessageId usersMessageId`.
- `protocol::ws::MessageAckEvent { type, usersMessageId, messageId, roomId, createdAt }`; `WsMessageType::MessageAck`. Зарегистрировано в `Parsing.hpp` (макрос, variant `MessageFromServer`, ветка switch).
- `ChatService::onSendMessage(sender, senderConn, request)`: `messageRepo_.create()` возвращает готовый `Message` (id + серверный createdAt — единый источник времени). СНАЧАЛА ack на `sendToConnection(senderConn, ...)`, ПОТОМ рассылка `NewMessage` через `sendToUserExcept(userId, &senderConn, payload)`. Другие устройства отправителя получают NewMessage и опознают своё по `senderId == userId`.
- Ошибки: `onSendMessage` возвращает `std::unexpected(ChatError)`, WsServer ловит и шлёт `ErrorMessage` через `HttpHelpers::mapChatErrorWs`. Отправку ошибок делает транспорт (WsServer), не ChatService.
- WsServer диспетчеризует через `std::visit` по перегрузкам `handle(conn, msg)`.

**Решение по типу id:** uint64 (не string), хотя для будущей дедупликации по clientMessageId строка была бы гибче (Этап 4 роадмапа).

См. [[server-architecture]], [[code-conventions]].
```

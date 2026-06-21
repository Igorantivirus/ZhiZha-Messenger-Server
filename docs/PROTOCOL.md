# Протокол сервера

Каждое протокольное имя имеет описание

```cpp

#include <Protocol/Common/Operatiopns.hpp>

struct Operation
{
    static constexpr HttpMethod method = HttpMethod::...;
    static constexpr Path path = API "...";
    struct PathParams
    {
        ...
    };
    struct QueryParams
    {
        ...
    };
    using Request = ...;
    using Response = ...;
};
```

## api: /api/v1/

1) GET - "/api/v1/health" - состояние сервера
2) GET - "/api/v1/info"   - информация о сервере

## auth: /api/v1/auth

1) POST - "/api/v1/auth/register"   - зарегистирироваться
2) POST - "/api/v1/auth/login"      - логин по логику и паролю
3) POST - "/api/v1/auth/refresh"    - обновить токены
4) POST - "/api/v1/auth/logout"     - завергшить сессию
5) POST - "/api/v1/auth/logout-all" - завершить все сессии

## rooms: /api/v1/rooms

1) GET     - "/api/v1/rooms"                        - Список моих комнат
2) POST    - "/api/v1/rooms"                        - Создать комнату
3) POST    - "/api/v1/rooms/loop"                   - Поиск открытых комнат

4) GET     - "/api/v1/rooms/<uint>"                 - Детали комнаты
5) DELETE  - "/api/v1/rooms/<uint>"                 - Удалить комнату
6) PUT     - "/api/v1/rooms/<uint>"                 - Обновить детали комнаты

7) GET     - "/api/v1/rooms/<uint>/members"         - Список участников
8) POST    - "/api/v1/rooms/<uint>/members"         - Пригласить участника

9) POST    - "/api/v1/rooms/<uint>/members/me"      - Войти в комнату
10) DELETE - "/api/v1/rooms/<uint>/members/me"      - Выйти из комнаты
11) DELETE - "/api/v1/rooms/<uint>/members/<uint>"  - Выгнать участника

## users: /api/v1/users

1) GET - "/api/v1/users/loop"   - поиск пользователей по запросу
2) PUT - "/api/v1/users/me"     - заменить информацию о себе 
3) GET - "/api/v1/users/me"     - получить информацию о себе
4) GET - "/api/v1/users/<uint>" - получить информацию о пользователе

## messages: /api/v1/messages

1) GET - "/api/v1/rooms/<uint>/messages" - получить сообщения
    

#pragma once

#include <Protocol/Ws.hpp>

// Контракт чата для транспортного слоя. WsServer сам разбирает WS-конверт
// и зовёт нужный метод; вся доменная логика (хранение, рассылка, права)
// живёт за этим интерфейсом.
//
// Заглушка: рабочая реализация будет добавлена позже.
class IChatService
{
public:
    virtual ~IChatService() = default;

    // Пользователь установил/закрыл WebSocket-подключение.
    virtual void onUserConnected(protocol::UserId userId) = 0;
    virtual void onUserDisconnected(protocol::UserId userId) = 0;

    // Пользователь прислал сообщение в чат.
    virtual void onSendMessage(protocol::UserId sender, const protocol::ws::SendMessageRequest &request) = 0;
};

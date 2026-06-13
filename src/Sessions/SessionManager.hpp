#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <crow/crow.h>

#include <Sessions/Session.hpp>
#include <Protocol/Types.hpp>

// Реестр живых WebSocket-подключений. Потокобезопасен: Crow обрабатывает
// соединения в нескольких потоках, поэтому каждый доступ под мьютексом.
//
// Один пользователь может держать несколько подключений (разные устройства),
// поэтому userId -> множество соединений.
class SessionManager
{
public:
    // Регистрирует подключение после успешного handshake. Вызывается из onopen.
    void add(crow::websocket::connection &conn, protocol::UserId userId)
    {
        std::unique_lock lock(mtx_);
        byConnection_.insert_or_assign(&conn, Session{.userId = userId, .connection = &conn});
        byUser_[userId].insert(&conn);
    }

    // Убирает подключение. Вызывается из onclose. Идемпотентно.
    void remove(crow::websocket::connection &conn)
    {
        std::unique_lock lock(mtx_);
        auto it = byConnection_.find(&conn);
        if (it == byConnection_.end())
            return;

        const protocol::UserId userId = it->second.userId;
        byConnection_.erase(it);

        auto userIt = byUser_.find(userId);
        if (userIt == byUser_.end())
            return;
        userIt->second.erase(&conn);
        if (userIt->second.empty())
            byUser_.erase(userIt);
    }

    // userId владельца подключения, если оно зарегистрировано.
    std::optional<protocol::UserId> userIdOf(crow::websocket::connection &conn) const
    {
        std::shared_lock lock(mtx_);
        auto it = byConnection_.find(&conn);
        if (it == byConnection_.end())
            return std::nullopt;
        return it->second.userId;
    }

    // Отправляет текст всем подключениям пользователя. Возвращает число адресатов.
    // Пригодится ChatService для доставки сообщений.
    std::size_t sendToUser(protocol::UserId userId, const std::string &text) const
    {
        // Копируем указатели под локом, send_text дёргаем уже без него,
        // чтобы не держать мьютекс на время сетевой операции.
        std::vector<crow::websocket::connection *> targets;
        {
            std::shared_lock lock(mtx_);
            auto it = byUser_.find(userId);
            if (it == byUser_.end())
                return 0;
            targets.assign(it->second.begin(), it->second.end());
        }
        for (auto *conn : targets)
            conn->send_text(text);
        return targets.size();
    }

    // Отправляет текст всем подключениям пользователя, КРОМE одного соединения
    // except. Нужно, чтобы NewMessage ушёл на все устройства отправителя, но
    // не вернулся в то самое соединение, что прислало сообщение (оно получит
    // Ack вместо NewMessage). Возвращает число фактических адресатов.
    std::size_t sendToUserExcept(protocol::UserId userId,
                                 const crow::websocket::connection *except,
                                 const std::string &text) const
    {
        std::vector<crow::websocket::connection *> targets;
        {
            std::shared_lock lock(mtx_);
            auto it = byUser_.find(userId);
            if (it == byUser_.end())
                return 0;
            for (auto *conn : it->second)
                if (conn != except)
                    targets.push_back(conn);
        }
        for (auto *conn : targets)
            conn->send_text(text);
        return targets.size();
    }

    // Отправляет текст одному конкретному соединению. Используется для Ack —
    // он адресован именно тому соединению, что прислало сообщение.
    void sendToConnection(crow::websocket::connection &conn, const std::string &text) const
    {
        conn.send_text(text);
    }

    bool isOnline(protocol::UserId userId) const
    {
        std::shared_lock lock(mtx_);
        return byUser_.contains(userId);
    }

private:
    mutable std::shared_mutex mtx_;
    std::unordered_map<crow::websocket::connection *, Session> byConnection_;
    std::unordered_map<protocol::UserId, std::unordered_set<crow::websocket::connection *>> byUser_;
};

#include "ChatServer.hpp"

#include <set>
#include <thread>
#include <utility>

#include "protocol/JsonPacker.hpp"
#include "protocol/JsonParser.hpp"

ChatServer::ChatServer(std::string serverName, std::string serverPublicKey, std::chrono::seconds registrationTimeout)
    : serverName_(std::move(serverName)),
      serverPublicKey_(std::move(serverPublicKey)),
      registrationTimeout_(registrationTimeout)
{
    rooms_.emplace(1, Room(1, Room::Type::Public));
    init();
}

void ChatServer::run(std::uint16_t port)
{
    server_.port(port).multithreaded().run();
}

void ChatServer::init()
{
    CROW_ROUTE(server_, "/info")([this]() {
        return infoServer();
    });

    CROW_WEBSOCKET_ROUTE(server_, "/ws")
        .onopen([this](crow::websocket::connection& conn) {
            onWebSocketOpen(conn);
        })
        .onmessage([this](crow::websocket::connection& conn, const std::string& data, bool isBinary) {
            onWebSocketMessage(conn, data, isBinary);
        })
        .onclose([this](crow::websocket::connection& conn, const std::string& reason, uint16_t closeCode) {
            onWebSocketClose(conn, reason, closeCode);
        });
}

std::string ChatServer::infoServer() const
{
    return JsonPacker::packServerInfo(true, serverName_);
}

void ChatServer::onWebSocketOpen(crow::websocket::connection& conn)
{
    CROW_LOG_INFO << "onWebSocketOpen(" << &conn << ")\n";
    auto user = std::make_shared<UserContext>();
    user->connection = &conn;
    user->connectionTime = std::chrono::steady_clock::now();

    {
        std::scoped_lock lock(stateMutex_);
        clients_[&conn] = user;
    }

    ServerHelloPayload helloPayload{};
    helloPayload.authorized = false;
    helloPayload.registrationTimeoutSeconds = static_cast<std::uint32_t>(registrationTimeout_.count());
    helloPayload.serverName = serverName_;
    conn.send_text(JsonPacker::packServerHello(helloPayload));

    std::thread([this, connection = &conn]() {
        std::this_thread::sleep_for(registrationTimeout_);
        disconnectIfRegistrationTimedOut(connection);
    }).detach();
}

void ChatServer::onWebSocketMessage(crow::websocket::connection& conn, const std::string& data, bool isBinary)
{
    CROW_LOG_INFO << "onWebSocketMessage(" << &conn << ", \""<< data << "\", " << isBinary << ")\n";
    try
    {
        if (isBinary)
        {
            conn.send_text(JsonPacker::packError({"error", "binary-not-supported", "Use JSON text messages only"}));
            return;
        }

        const auto user = findUser(&conn);
        if (user == nullptr)
        {
            conn.close("unknown connection", crow::websocket::CloseStatusCode::PolicyViolated);
            return;
        }

        if (user->closing.load())
        {
            conn.close("closing", crow::websocket::CloseStatusCode::EndpointGoingAway);
            return;
        }

        const auto jsonPayload = JsonParser::parseJson(data);
        if (!jsonPayload.has_value())
        {
            conn.send_text(JsonPacker::packError({"error", "invalid-json", "Payload must be valid JSON object"}));
            return;
        }

        const auto type = JsonParser::parseMessageType(*jsonPayload);
        if (!type.has_value())
        {
            conn.send_text(JsonPacker::packError({"error", "invalid-json", "Field 'type' is required"}));
            return;
        }

        if (*type == "register")
        {
            const auto request = JsonParser::parseRegisterRequest(*jsonPayload);
            if (!request.has_value())
            {
                conn.send_text(
                    JsonPacker::packError({"error", "public-key-required", "Field 'public-key' must be string"}));
                return;
            }

            handleRegistrationMessage(user, *request);
            return;
        }

        if (!user->authorized.load())
        {
            conn.send_text(JsonPacker::packError({"error", "not-authorized", "Register first"}));
            return;
        }

        if (*type == "chat-msg")
        {
            const auto request = JsonParser::parseChatMessageRequest(*jsonPayload);
            if (!request.has_value())
            {
                conn.send_text(JsonPacker::packError(
                    {"error", "invalid-chat-payload", "user-id, chat-id and message are required"}));
                return;
            }

            handleChatMessage(user, *request);
            return;
        }

        if (*type == "create-room")
        {
            const auto request = JsonParser::parseCreateRoomRequest(*jsonPayload);
            if (!request.has_value())
            {
                conn.send_text(JsonPacker::packError(
                    {"error", "invalid-create-room-payload", "user-id and participant-user-ids are required"}));
                return;
            }

            handleCreateRoomRequest(user, *request);
            return;
        }

        if (*type == "leave-room")
        {
            const auto request = JsonParser::parseLeaveRoomRequest(*jsonPayload);
            if (!request.has_value())
            {
                conn.send_text(
                    JsonPacker::packError({"error", "invalid-leave-room-payload", "user-id and chat-id are required"}));
                return;
            }

            handleLeaveRoomRequest(user, *request);
            return;
        }

        conn.send_text(JsonPacker::packError({"error", "unknown-message-type", "Unsupported message type"}));
    }
    catch (const std::bad_alloc&)
    {
        conn.close("out of memory", crow::websocket::CloseStatusCode::UnexpectedCondition);
    }
    catch (const std::exception&)
    {
        conn.close("internal error", crow::websocket::CloseStatusCode::UnexpectedCondition);
    }
}

void ChatServer::onWebSocketClose(crow::websocket::connection& conn, const std::string&, uint16_t)
{
    CROW_LOG_INFO << "onWebSocketClose(" << &conn << ")\n";
    std::scoped_lock lock(stateMutex_);
    const auto clientIt = clients_.find(&conn);
    if (clientIt == clients_.end())
    {
        return;
    }

    const auto user = clientIt->second;
    if (user != nullptr && user->userId != 0)
    {
        usersById_.erase(user->userId);
    }

    for (const auto roomId : user->roomIds)
    {
        const auto roomIt = rooms_.find(roomId);
        if (roomIt != rooms_.end())
        {
            roomIt->second.removeUser(user);
            if (roomId != 1 && roomIt->second.empty())
            {
                rooms_.erase(roomIt);
            }
        }
    }

    clients_.erase(clientIt);
}

void ChatServer::handleRegistrationMessage(const UserContextPtr& user, const ClientRegisterRequest& request)
{
    std::vector<IDType> usersChats;
    ServerRegistrationPayload response{};
    {
        std::scoped_lock lock(stateMutex_);
        if (user->closing.load())
        {
            return;
        }

        if (user->authorized.load())
        {
            user->connection->send_text(JsonPacker::packError({"register-error", "already-registered", "Already registered"}));
            return;
        }
        if(request.username.empty())
        {
            user->connection->send_text(JsonPacker::packError({"register-error", "empty-username", "Username is empty"}));
            return;
        }
        if(request.password.empty())
        {
            user->connection->send_text(JsonPacker::packError({"register-error", "empty-password", "Password is empty"}));
            return;
        }

        //Проверка, что нет пользователя с таким именем
        for(const auto& [ID, userByID] : usersById_)
        {
            if(ID == user->userId)
                continue;
            if(!userByID->authorized)//Если пользователь не авторизован -- то тот, кто раньше занял имя, того и тапки
                continue;
            if(userByID->username == request.username)
            {
                user->connection->send_text(JsonPacker::packError({"register-error", "username-busy", "There is a user with that name"}));
                return;
            }
        }

        if(!user->password.empty() && user->password != request.password)
        {
            user->connection->send_text(JsonPacker::packError({"register-error", "wrong-password", "Invalid password"}));
            return;
        }

        user->publicKey = request.publicKey;
        user->username = request.username;
        user->password = request.password;
        user->userId = nextUserId_.fetch_add(1);
        user->authorized.store(true);

        usersById_[user->userId] = user;

        auto roomIt = rooms_.find(1);
        if (roomIt != rooms_.end())
        {
            roomIt->second.addUser(user);
            user->roomIds.insert(1);
        }

        for (const auto roomId : user->roomIds)
        {
            usersChats.push_back(roomId);
        }

        response.registered = true;
        response.userId = user->userId;
        response.serverPublicKey = serverPublicKey_;
        response.usersChats = std::move(usersChats);
        response.serverName = serverName_;
    }

    user->connection->send_text(JsonPacker::packRegistration(response));
}

void ChatServer::handleChatMessage(const UserContextPtr& user, const ClientChatMessageRequest& request)
{
    if (request.userId != user->userId)
    {
        user->connection->send_text(JsonPacker::packError({"error", "wrong-user-id", "Invalid user-id"}));
        return;
    }

    std::scoped_lock lock(stateMutex_);
    if (!user->roomIds.contains(request.chatId))
    {
        user->connection->send_text(JsonPacker::packError({"error", "chat-access-denied", "No access to this chat"}));
        return;
    }

    const auto roomIt = rooms_.find(request.chatId);
    if (roomIt == rooms_.end())
    {
        user->connection->send_text(JsonPacker::packError({"error", "chat-not-found", "Chat not found"}));
        return;
    }

    ServerChatMessagePayload response{};
    response.userId = user->userId;
    response.userName = user->username;
    response.chatId = request.chatId;
    response.message = request.message;
    response.serverMessageId = nextServerMessageId_.fetch_add(1);

    roomIt->second.broadcast(JsonPacker::packChatMessage(response));
}

void ChatServer::handleCreateRoomRequest(const UserContextPtr& user, const ClientCreateRoomRequest& request)
{
    if (request.userId != user->userId)
    {
        user->connection->send_text(JsonPacker::packError({"error", "wrong-user-id", "Invalid user-id"}));
        return;
    }

    std::vector<IDType> participants;
    std::scoped_lock lock(stateMutex_);

    const IDType roomId = nextRoomId_.fetch_add(1);
    Room room(roomId, request.isPrivate ? Room::Type::Private : Room::Type::Public);

    room.addUser(user);
    user->roomIds.insert(roomId);
    participants.push_back(user->userId);

    std::set<IDType> uniqueIds(request.participantUserIds.begin(), request.participantUserIds.end());
    uniqueIds.erase(user->userId);
    for (const auto participantId : uniqueIds)
    {
        const auto participantIt = usersById_.find(participantId);
        if (participantIt == usersById_.end())
        {
            continue;
        }

        auto& participant = participantIt->second;
        if (participant == nullptr || !participant->authorized.load())
        {
            continue;
        }

        room.addUser(participant);
        participant->roomIds.insert(roomId);
        participants.push_back(participantId);
    }

    rooms_.emplace(roomId, std::move(room));

    ServerRoomCreatedPayload response{};
    response.created = true;
    response.chatId = roomId;
    response.participantUserIds = participants;

    user->connection->send_text(JsonPacker::packRoomCreated(response));
}

void ChatServer::handleLeaveRoomRequest(const UserContextPtr& user, const ClientLeaveRoomRequest& request)
{
    if (request.userId != user->userId)
    {
        user->connection->send_text(JsonPacker::packError({"error", "wrong-user-id", "Invalid user-id"}));
        return;
    }

    std::scoped_lock lock(stateMutex_);
    const auto roomIt = rooms_.find(request.chatId);
    if (roomIt == rooms_.end())
    {
        user->connection->send_text(JsonPacker::packError({"error", "chat-not-found", "Chat not found"}));
        return;
    }

    if (!user->roomIds.contains(request.chatId))
    {
        user->connection->send_text(JsonPacker::packError({"error", "chat-access-denied", "No access to this chat"}));
        return;
    }

    roomIt->second.removeUser(user);
    user->roomIds.erase(request.chatId);

    if (request.chatId != 1 && roomIt->second.empty())
    {
        rooms_.erase(roomIt);
    }

    ServerRoomLeftPayload response{};
    response.left = true;
    response.userId = user->userId;
    response.chatId = request.chatId;

    user->connection->send_text(JsonPacker::packRoomLeft(response));
}

void ChatServer::disconnectIfRegistrationTimedOut(crow::websocket::connection* connection)
{
    UserContextPtr user;
    {
        std::scoped_lock lock(stateMutex_);
        const auto it = clients_.find(connection);
        if (it == clients_.end() || it->second->authorized.load())
        {
            return;
        }
        user = it->second;
        user->closing.store(true);
    }

    if (user != nullptr && user->connection != nullptr)
    {
        user->connection->close("registration timeout", crow::websocket::CloseStatusCode::PolicyViolated);
    }
}

UserContextPtr ChatServer::findUser(crow::websocket::connection* connection)
{
    std::scoped_lock lock(stateMutex_);
    const auto it = clients_.find(connection);
    if (it == clients_.end())
    {
        return nullptr;
    }
    return it->second;
}

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

#include <crow.h>

#include "core/Room.hpp"
#include "protocol/JsonMessages.hpp"

class ChatServer
{
public:
    ChatServer(std::string serverName, std::string serverPublicKey, std::chrono::seconds registrationTimeout);

    void run(std::uint16_t port);

private:
    void init();
    std::string infoServer() const;

    void onWebSocketOpen(crow::websocket::connection& conn);
    void onWebSocketMessage(crow::websocket::connection& conn, const std::string& data, bool isBinary);
    void onWebSocketClose(crow::websocket::connection& conn, const std::string& reason, uint16_t closeCode);

    void handleRegistrationMessage(const UserContextPtr& user, const ClientRegisterRequest& request);
    void handleChatMessage(const UserContextPtr& user, const ClientChatMessageRequest& request);
    void handleCreateRoomRequest(const UserContextPtr& user, const ClientCreateRoomRequest& request);
    void handleLeaveRoomRequest(const UserContextPtr& user, const ClientLeaveRoomRequest& request);
    void handleDataRequest(const UserContextPtr& user, const ClientDataRequest& request);

    void disconnectIfRegistrationTimedOut(crow::websocket::connection* connection);
    UserContextPtr findUser(crow::websocket::connection* connection);

private:
    std::string serverName_;
    std::string serverPublicKey_;
    std::chrono::seconds registrationTimeout_;

    crow::SimpleApp server_;

    std::unordered_map<IDType, Room> rooms_;
    std::unordered_map<crow::websocket::connection*, UserContextPtr> clients_;
    std::unordered_map<IDType, UserContextPtr> usersById_;
    std::mutex stateMutex_;

    std::atomic<IDType> nextUserId_{1};
    std::atomic<IDType> nextRoomId_{2};
    std::atomic<std::uint64_t> nextServerMessageId_{1};
};


#pragma once

#include <string>

class KeyGenerator
{
public:
    struct WsUrl
    {
        std::string host;
        std::string port;
        std::string target;
    };

    static std::string generateKey(const std::string &ip, const int port);
    static WsUrl fromKey(const std::string& key);
};
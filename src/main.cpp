#include <chrono>
#include <cstdlib>

#include "ChatServer.hpp"

#include "core/KeyGenerator.hpp"

int main()
{
#if _WIN32
    std::system("chcp 65001 > nul");
#endif

    ChatServer server("Messenger2 Server", "server-public-key-stub", std::chrono::seconds(20));
    std::cout << "Server key: " << KeyGenerator::generateKey("10.241.69.217", 18080) << '\n';
    server.run(18080);

    return 0;
}
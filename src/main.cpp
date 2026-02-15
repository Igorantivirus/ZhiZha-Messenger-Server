#include <chrono>
#include <cstdlib>

#include "ChatServer.hpp"

#include <boost/beast.hpp>
#include <boost/asio.hpp>

int main()
{
#if _WIN32
    std::system("chcp 65001 > nul");
#endif

    ChatServer server("Messenger2 Server", "server-public-key-stub", std::chrono::seconds(20));
    server.run(18080);

    return 0;
}

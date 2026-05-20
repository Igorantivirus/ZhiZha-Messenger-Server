#include <cstdlib>

#include <App/ServerApplication.hpp>

#include <Auth/impl/DummyPasswordHasher.hpp>

int main()
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    ServerApplication app;
    app.run(8080);
    return 0;
}
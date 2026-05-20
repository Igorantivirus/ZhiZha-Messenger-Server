#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <App/ServerApplication.hpp>
#include <App/ServerConfig.hpp>

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif

    // Первый аргумент — путь до конфиг-файла. По умолчанию config.json рядом.
    const std::string configPath = (argc > 1) ? argv[1] : "config.json";

    try
    {
        ServerConfig config = ServerConfig::loadFromFile(configPath);
        ServerApplication app(std::move(config));
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

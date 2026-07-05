#include <cstdlib>
#include <exception>
#include <iostream>

#include <App/Configs/Parsing.hpp>
#include <App/ServerApplication.hpp>
#include <Utils/ConfigReader.hpp>

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif

    const std::string configPath = (argc > 1) ? argv[1] : "config.json";

    try
    {
        app::ServerConfig config = utils::ConfigReader::loadFromFile<app::ServerConfig>(configPath);
        app::ServerApplication app(std::move(config));
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
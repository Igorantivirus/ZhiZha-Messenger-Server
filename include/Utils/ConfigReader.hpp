#pragma once

#include <fstream>

#include <nlohmann/json.hpp>

namespace utils
{
class ConfigReader
{
public:
    template <class Config>
    static Config loadFromFile(const std::string &path)
    {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("Cannot open config file: " + path);
        nlohmann::json json;
        file >> json;
        return json.get<Config>();
    }
};
} // namespace utils
#pragma once

#include <cstdint>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include <Validation/StringRule.hpp>

// Единый конфиг сервера. Грузится из JSON-файла (путь — первым аргументом
// командной строки) и целиком определяет поведение: порт, сроки токенов,
// файл БД и правила валидации. Никаких магических чисел в коде —
// ServerApplication настраивает всё отсюда.

// Правила валидации именованных полей. Каждое поле — отдельный StringRule.
struct ValidationConfig
{
    validation::StringRule username{
        .minLength = 4, .maxLength = 32, .mustStartWithLetter = true, .allowedSpecialSymbols = "-_"};
    validation::StringRule password{
        .minLength = 8, .maxLength = 64, .allowedSpecialSymbols = "-_()@#$%^&*!?=+/"};
    validation::StringRule displayName{
        .minLength = 1, .maxLength = 48, .allowedSpecialSymbols = "-_ ."};
    validation::StringRule roomName{
        .minLength = 1, .maxLength = 64, .allowedSpecialSymbols = "-_ ."};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ValidationConfig, username, password, displayName, roomName)

struct ServerConfig
{
    std::uint16_t port = 8080;
    std::time_t accessTtl = 900;   // 15 минут
    std::time_t refreshTtl = 7200; // 2 часа
    std::string databaseFile = "zhizha.db";
    ValidationConfig validation;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ServerConfig, port, accessTtl, refreshTtl, databaseFile, validation)

    // Загружает конфиг из JSON-файла. Отсутствующие ключи берут значения по
    // умолчанию (см. поля выше), поэтому файл может быть частичным.
    static ServerConfig loadFromFile(const std::string &path)
    {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("Cannot open config file: " + path);

        nlohmann::json json;
        file >> json; // бросит при битом JSON
        return json.get<ServerConfig>();
    }
    
};


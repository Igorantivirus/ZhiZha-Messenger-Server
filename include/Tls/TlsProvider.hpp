#pragma once

#include <filesystem>
#include <string>

#include <App/Configs/TlsConfig.hpp>

namespace tls
{

// Готовый TLS-материал для запуска сервера. Пути указывают на PEM-файлы
// (Crow принимает пути, не содержимое), fingerprint — SHA-256 от DER
// сертификата в нижнем hex — именно он идёт во фрагмент ссылки-приглашения.
struct TlsMaterial
{
    std::string certFile;
    std::string keyFile;
    std::string fingerprint;         // sha-256(DER), 64 hex-символа, lowercase
    bool selfSigned = false;         // true, если пара сгенерирована сервером
    bool fingerprintChanged = false; // fingerprint отличается от прошлого запуска
};

// Единственная ответственность модуля: по TLS-фрагменту конфига выдать
// пару сертификат+ключ и её fingerprint. Дальше сервер настраивается сам.
//
// Правила:
//  - certFile и keyFile заданы -> загрузить и проверить (пара согласована,
//    срок не истёк). Невалидны -> исключение (fail-fast: молча подменять
//    явно указанный сертификат самоподписанным опасно).
//  - оба пути пусты -> искать ранее сгенерированную пару в workDir;
//    есть и валидна -> использовать (fingerprint стабилен между запусками);
//    нет -> сгенерировать самоподписанную (EC P-256, ~10 лет) и сохранить.
//  - задан только один из путей -> исключение (ошибка конфигурации).
//
// Детекция смены ссылки: fingerprint прошлого запуска хранится в
// workDir/tls-cert.sha256; если текущий отличается — fingerprintChanged=true
// (сервер печатает плашку «ссылка изменилась»). Файл обновляется всегда.
class TlsProvider
{
public:
    TlsProvider() = delete;

    static TlsMaterial prepare(const app::TlsConfig &config,
                               const std::filesystem::path &workDir);
};

} // namespace tls

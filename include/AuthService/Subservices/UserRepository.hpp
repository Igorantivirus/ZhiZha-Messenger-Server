#pragma once

#include <expected>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Utils/Types.hpp>

#include <AuthService/Types/AuthError.hpp>
#include <AuthService/Types/User.hpp>

namespace auth
{

class UserRepository
{
public:
    explicit UserRepository(SQLite::Database &db);

    // registerTime ставит сам репозиторий.
    // Ошибки: UsernameTaken — если username уже занят (UNIQUE).
    std::expected<utils::UserId, AuthError> create(
        const std::string &username,
        const std::string &displayName,
        const std::string &passwordHash,
        const std::time_t birthDate,
        const utils::Country country);

    std::optional<User> findUserByUsername(const std::string &username) const;
    std::optional<User> findUserById(const utils::UserId id) const;
    std::vector<User> findUsersByQuery(const std::string &query, const unsigned limit) const;

    // Пакетная выборка по списку id. Дубликаты в ids игнорируются: каждый
    // пользователь возвращается не более одного раза. Порядок результата
    // не гарантируется.
    std::vector<User> findByIds(std::span<const utils::UserId> ids) const;

    void remove(const utils::UserId userId);

    // Обновляет редактируемые поля профиля. Пароль НЕ трогает.
    // Ошибки: UserNotFound — нет такого id; UsernameTaken — новый username
    // уже занят другим пользователем (UNIQUE).
    std::expected<void, AuthError> updateInfo(
        const utils::UserId userId,
        const std::string &username,
        const std::string &displayName,
        const std::time_t birthDate,
        const utils::Country country);

    // Отдельно — смена пароля.
    // Ошибки: UserNotFound — нет такого id.
    std::expected<void, AuthError> updatePassword(
        const utils::UserId userId,
        const std::string &newPasswordHash);

private:
    SQLite::Database &db_;
};

} // namespace auth

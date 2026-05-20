#pragma once

#include <ctime>
#include <memory>
#include <optional>

#include <SQLiteCpp/SQLiteCpp.h>

#include <Auth/Interfaces/IUserRepository.hpp>

class SQLiteUserRepository : public IUserRepository
{
public:
    explicit SQLiteUserRepository(std::shared_ptr<SQLite::Database> db)
        : db_(std::move(db))
    {
        db_->exec(CREATE_TABLE_COMMAND.data());
    }

    std::optional<User> findUserByUsername(const std::string &username) const override
    {
        SQLite::Statement query(*db_, SELECT_BY_USERNAME.data());
        query.bind(1, username);

        if (query.executeStep())
            return rowToUser(query);
        return std::nullopt;
    }

    std::optional<User> findUserById(const UserId id) const override
    {
        SQLite::Statement query(*db_, SELECT_BY_ID.data());
        query.bind(1, static_cast<int64_t>(id)); // UserId = uint64_t, bind ожидает int64_t

        if (query.executeStep())
            return rowToUser(query);
        return std::nullopt;
    }

    UserId create(const std::string &username, const std::string &displayName, const std::string &passwordHash) override
    {
        const std::int64_t now = static_cast<int64_t>(std::time(nullptr));

        SQLite::Statement insert(*db_, INSERT_USER_COMMAND.data());
        insert.bind(1, username);
        insert.bind(2, passwordHash);
        insert.bind(3, now);
        insert.bind(4, displayName);

        insert.exec();
        return db_->getLastInsertRowid(); // возвращает rowid (он же id)
    }

    bool updateUsername(const UserId id, const std::string newUsername) override
    {
        SQLite::Statement stmt(*db_, UPDATE_USERNAME_COMMAND.data());
        stmt.bind(1, newUsername);
        stmt.bind(2, id);

        return stmt.exec() > 0;
    }

    bool updatePasswordHash(const UserId id, const std::string newPasswordHash) override
    {
        SQLite::Statement stmt(*db_, UPDATE_PASSWORD_HASH_COMMAND.data());
        stmt.bind(1, newPasswordHash);
        stmt.bind(2, id);

        return stmt.exec() > 0;
    }

private:
    // Преобразование строки результата в объект User
    static User rowToUser(SQLite::Statement &query)
    {
        User user;
        user.id = query.getColumn(0).getInt64();
        user.username = query.getColumn(1).getString();
        user.passwordHash = query.getColumn(2).getString();
        user.registerTime = query.getColumn(3).getInt64();
        user.displayeName = query.getColumn(4).getString();
        return user;
    }

    // Общая БД всего сервера — владение разделяется через shared_ptr.
    std::shared_ptr<SQLite::Database> db_;

    // SQL-команды
    static constexpr std::string_view CREATE_TABLE_COMMAND =
        "CREATE TABLE IF NOT EXISTS users("
        "id INTEGER PRIMARY KEY,"
        "username TEXT NOT NULL UNIQUE,"
        "passwordHash TEXT NOT NULL,"
        "registerTime INTEGER NOT NULL,"
        "displayeName TEXT NOT NULL"
        ")";

    static constexpr std::string_view INSERT_USER_COMMAND =
        "INSERT INTO users (username, passwordHash, registerTime, displayeName) "
        "VALUES (?, ?, ?, ?)";

    static constexpr std::string_view SELECT_BY_USERNAME =
        "SELECT id, username, passwordHash, registerTime, displayeName "
        "FROM users WHERE username = ?";

    static constexpr std::string_view SELECT_BY_ID =
        "SELECT id, username, passwordHash, registerTime, displayeName "
        "FROM users WHERE id = ?";

    static constexpr std::string_view UPDATE_USERNAME_COMMAND =
        "UPDATE users SET username = ? WHERE id = ?";

    static constexpr std::string_view UPDATE_PASSWORD_HASH_COMMAND =
        "UPDATE users SET passwordHash = ? WHERE id = ?";
};
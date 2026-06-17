#pragma once

#include <algorithm>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "magic_enum/magic_enum.hpp"
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

    std::optional<User> findUserById(const protocol::UserId id) const override
    {
        SQLite::Statement query(*db_, SELECT_BY_ID.data());
        query.bind(1, static_cast<int64_t>(id)); // UserId = uint64_t, bind ожидает int64_t

        if (query.executeStep())
            return rowToUser(query);
        return std::nullopt;
    }

    std::vector<User> findUsersByQuery(std::string query, unsigned limit) const override
    {
        if (query.empty() || limit == 0)
            return std::vector<User>{};

        const std::string likePattern = "%" + escapeLike(query) + "%";

        std::vector<User> results;
        results.reserve(limit);
        std::unordered_set<protocol::UserId> seenIds;

        // Первый проход: поиск по displayeName
        appendMatches(SELECT_BY_DISPLAYNAME_LIKE, likePattern, query, limit, results, seenIds, /*byDisplayName=*/true);

        // Второй проход: добор по username, если не хватило
        if (results.size() < limit)
            appendMatches(SELECT_BY_USERNAME_LIKE, likePattern, query, limit, results, seenIds, /*byDisplayName=*/false);

        return results;
    }

    protocol::UserId create(const std::string &username, const std::string &displayName, const std::string &passwordHash, const std::time_t birthDate, const protocol::users::Country country) override
    {
        // registerTime — серверная истина, ставим сами (клиент не присылает).
        const std::int64_t now = static_cast<int64_t>(std::time(nullptr));

        SQLite::Statement insert(*db_, INSERT_USER_COMMAND.data());
        insert.bind(1, username);
        insert.bind(2, passwordHash);
        insert.bind(3, now);
        insert.bind(4, displayName);
        insert.bind(5, static_cast<std::int64_t>(birthDate));
        insert.bind(6, countryToString(country));

        insert.exec();
        return db_->getLastInsertRowid(); // возвращает rowid (он же id)
    }

    bool updateUsername(const protocol::UserId id, const std::string newUsername) override
    {
        SQLite::Statement stmt(*db_, UPDATE_USERNAME_COMMAND.data());
        stmt.bind(1, newUsername);
        stmt.bind(2, id);

        return stmt.exec() > 0;
    }

    bool updatePasswordHash(const protocol::UserId id, const std::string newPasswordHash) override
    {
        SQLite::Statement stmt(*db_, UPDATE_PASSWORD_HASH_COMMAND.data());
        stmt.bind(1, newPasswordHash);
        stmt.bind(2, id);

        return stmt.exec() > 0;
    }

    bool updateEditableInfo(const protocol::UserId id, const std::string &newUsername, const std::string &newDisplayName, const std::time_t birthDate, const protocol::users::Country country) override
    {
        SQLite::Statement stmt(*db_, UPDATE_EDITABLE_INFO_COMMAND.data());
        stmt.bind(1, newUsername);
        stmt.bind(2, newDisplayName);
        stmt.bind(3, static_cast<std::int64_t>(birthDate));
        stmt.bind(4, countryToString(country));
        stmt.bind(5, static_cast<std::int64_t>(id));

        return stmt.exec() > 0;
    }

private:
    // Общая БД всего сервера — владение разделяется через shared_ptr.
    std::shared_ptr<SQLite::Database> db_;

private:
    // Загружает кандидатов из БД, ранжирует и добавляет в results до достижения limit.
    // Дедупликация по seenIds — чтобы юзер не попал дважды из двух проходов.
    void appendMatches(
        std::string_view sql,
        const std::string &likePattern,
        const std::string &query,
        unsigned limit,
        std::vector<User> &results,
        std::unordered_set<protocol::UserId> &seenIds,
        bool byDisplayName) const
    {
        struct Scored
        {
            User user;
            int score;
        };
        std::vector<Scored> candidates;

        SQLite::Statement stmt(*db_, sql.data());
        stmt.bind(1, likePattern);

        while (stmt.executeStep())
        {
            User user = rowToUser(stmt);
            if (seenIds.count(user.id))
                continue;

            const std::string &field = byDisplayName ? user.displayeName : user.username;
            candidates.push_back({std::move(user), scoreMatch(field, query)});
        }

        // Сортировка по убыванию score — лучшие совпадения вперёд.
        std::sort(candidates.begin(), candidates.end(),
                  [](const Scored &a, const Scored &b)
        {
            return a.score > b.score;
        });

        for (auto &c : candidates)
        {
            if (results.size() >= limit)
                break;
            seenIds.insert(c.user.id);
            results.push_back(std::move(c.user));
        }
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
        user.birthDate = static_cast<std::time_t>(query.getColumn(5).getInt64());
        user.country = countryFromString(query.getColumn(6).getString());
        return user;
    }

    // Country хранится строкой (имя enum через magic_enum), как роли/политики комнат.
    static std::string countryToString(protocol::users::Country c)
    {
        return std::string(magic_enum::enum_name(c));
    }
    static protocol::users::Country countryFromString(const std::string &s)
    {
        auto casted = magic_enum::enum_cast<protocol::users::Country>(s);
        return casted ? casted.value() : protocol::users::Country::None;
    }

    // Score: чем раньше встретилось совпадение и чем короче поле — тем выше.
    // Точное совпадение с начала строки даёт максимум.
    static int scoreMatch(const std::string &field, const std::string &query)
    {
        const auto pos = field.find(query);
        if (pos == std::string::npos)
            return 0; // не должно случаться — LIKE уже отфильтровал, но на всякий

        int score = 1000;
        score -= static_cast<int>(pos) * 10;     // штраф за позицию совпадения
        score -= static_cast<int>(field.size()); // штраф за длину поля (короче = релевантнее)
        if (pos == 0)
            score += 500; // бонус за совпадение с начала
        return score;
    }

    // Экранирование спецсимволов LIKE (%, _, \) — чтобы поиск "100%" не превращался в wildcard.
    static std::string escapeLike(const std::string &s)
    {
        std::string result;
        result.reserve(s.size());
        for (char c : s)
        {
            if (c == '%' || c == '_' || c == '\\')
                result += '\\';
            result += c;
        }
        return result;
    }

private: // SQL-команды
    static constexpr std::string_view CREATE_TABLE_COMMAND =
        "CREATE TABLE IF NOT EXISTS users("
        "id INTEGER PRIMARY KEY,"
        "username TEXT NOT NULL UNIQUE,"
        "passwordHash TEXT NOT NULL,"
        "registerTime INTEGER NOT NULL,"
        "displayeName TEXT NOT NULL,"
        "birthDate INTEGER NOT NULL,"
        "country TEXT NOT NULL"
        ")";

    static constexpr std::string_view INSERT_USER_COMMAND =
        "INSERT INTO users (username, passwordHash, registerTime, displayeName, birthDate, country) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    static constexpr std::string_view SELECT_BY_USERNAME =
        "SELECT id, username, passwordHash, registerTime, displayeName, birthDate, country "
        "FROM users WHERE username = ?";

    static constexpr std::string_view SELECT_BY_ID =
        "SELECT id, username, passwordHash, registerTime, displayeName, birthDate, country "
        "FROM users WHERE id = ?";

    static constexpr std::string_view UPDATE_USERNAME_COMMAND =
        "UPDATE users SET username = ? WHERE id = ?";

    static constexpr std::string_view UPDATE_PASSWORD_HASH_COMMAND =
        "UPDATE users SET passwordHash = ? WHERE id = ?";

    static constexpr std::string_view UPDATE_EDITABLE_INFO_COMMAND =
        "UPDATE users SET username = ?, displayeName = ?, birthDate = ?, country = ? "
        "WHERE id = ?";

    static constexpr std::string_view SELECT_BY_DISPLAYNAME_LIKE =
        "SELECT id, username, passwordHash, registerTime, displayeName, birthDate, country "
        "FROM users WHERE displayeName LIKE ? ESCAPE '\\'";

    static constexpr std::string_view SELECT_BY_USERNAME_LIKE =
        "SELECT id, username, passwordHash, registerTime, displayeName, birthDate, country "
        "FROM users WHERE username LIKE ? ESCAPE '\\'";
};
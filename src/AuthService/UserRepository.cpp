#include <AuthService/Subservices/UserRepository.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#include <SQLiteCpp/Exception.h>
#include <magic_enum/magic_enum.hpp>
#include <sqlite3.h>

#include <Utils/Time.hpp>

namespace auth
{

namespace
{

// SQL.
constexpr std::string_view CREATE_TABLE =
    "CREATE TABLE IF NOT EXISTS users ("
    "id           INTEGER PRIMARY KEY AUTOINCREMENT, "
    "username     TEXT    NOT NULL UNIQUE, "
    "passwordHash TEXT    NOT NULL, "
    "displayName TEXT    NOT NULL, "
    "country      TEXT    NOT NULL, "
    "birthDate    INTEGER NOT NULL, "
    "registerTime INTEGER NOT NULL"
    ")";

constexpr std::string_view INSERT_USER =
    "INSERT INTO users (username, passwordHash, displayName, country, birthDate, registerTime) "
    "VALUES (?, ?, ?, ?, ?, ?)";

constexpr std::string_view SELECT_BY_ID =
    "SELECT id, passwordHash, username, displayName, country, registerTime, birthDate "
    "FROM users WHERE id = ?";

constexpr std::string_view SELECT_BY_USERNAME =
    "SELECT id, passwordHash, username, displayName, country, registerTime, birthDate "
    "FROM users WHERE username = ?";

constexpr std::string_view SELECT_BY_QUERY =
    "SELECT id, passwordHash, username, displayName, country, registerTime, birthDate "
    "FROM users WHERE username LIKE ? LIMIT ?";

constexpr std::string_view SELECT_BY_IDS_PREFIX =
    "SELECT id, passwordHash, username, displayName, country, registerTime, birthDate "
    "FROM users WHERE id IN ";

constexpr std::string_view DELETE_BY_ID =
    "DELETE FROM users WHERE id = ?";

constexpr std::string_view UPDATE_INFO =
    "UPDATE users SET username = ?, displayName = ?, country = ?, birthDate = ? "
    "WHERE id = ?";

constexpr std::string_view UPDATE_PASSWORD =
    "UPDATE users SET passwordHash = ? WHERE id = ?";

// Хранение enum'ов: имя перечислителя строкой. Стабильно к перестановке
// значений в enum, читаемо при ручном просмотре БД.
std::string countryToString(const utils::Country c)
{
    return std::string(magic_enum::enum_name(c));
}
utils::Country countryFromString(const std::string &s)
{
    auto v = magic_enum::enum_cast<utils::Country>(s);
    return v.value_or(utils::Country::None);
}

User rowToUser(SQLite::Statement &stmt)
{
    return User{
        .id = static_cast<utils::UserId>(stmt.getColumn(0).getInt64()),
        .passwordHash = stmt.getColumn(1).getString(),
        .username = stmt.getColumn(2).getString(),
        .displayName = stmt.getColumn(3).getString(),
        .country = countryFromString(stmt.getColumn(4).getString()),
        .registerTime = static_cast<std::time_t>(stmt.getColumn(5).getInt64()),
        .birthDate = static_cast<std::time_t>(stmt.getColumn(6).getInt64())};
}

} // namespace

UserRepository::UserRepository(SQLite::Database &db)
    : db_(db)
{
    db_.exec(std::string(CREATE_TABLE));
}

std::expected<utils::UserId, AuthError> UserRepository::create(
    const std::string &username,
    const std::string &displayName,
    const std::string &passwordHash,
    const std::time_t birthDate,
    const utils::Country country)
{
    const std::time_t now = utils::getCurrentTime();

    SQLite::Statement stmt(db_, std::string(INSERT_USER));
    stmt.bind(1, username);
    stmt.bind(2, passwordHash);
    stmt.bind(3, displayName);
    stmt.bind(4, countryToString(country));
    stmt.bind(5, static_cast<std::int64_t>(birthDate));
    stmt.bind(6, static_cast<std::int64_t>(now));

    // UNIQUE(username) — ожидаемая бизнес-ситуация: маппим в AuthError.
    // Прочие сбои БД пропускаем наверх как исключение (это уже инфра).
    try
    {
        stmt.exec();
    }
    catch (const SQLite::Exception &e)
    {
        if (e.getExtendedErrorCode() == SQLITE_CONSTRAINT_UNIQUE)
            return std::unexpected(AuthError::UsernameTaken);
        throw;
    }

    return static_cast<utils::UserId>(db_.getLastInsertRowid());
}

std::optional<User> UserRepository::findUserByUsername(const std::string &username) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_USERNAME));
    stmt.bind(1, username);
    if (!stmt.executeStep())
        return std::nullopt;
    return rowToUser(stmt);
}

std::optional<User> UserRepository::findUserById(const utils::UserId id) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_ID));
    stmt.bind(1, static_cast<std::int64_t>(id));
    if (!stmt.executeStep())
        return std::nullopt;
    return rowToUser(stmt);
}

std::vector<User> UserRepository::findUsersByQuery(const std::string &query, const unsigned limit) const
{
    SQLite::Statement stmt(db_, std::string(SELECT_BY_QUERY));
    // %query% — простой контейнс. SQL-injection не пройдёт (параметр), но
    // %/_ в самом query сработают как LIKE-wildcards — для прототипа ок.
    stmt.bind(1, "%" + query + "%");
    stmt.bind(2, static_cast<std::int64_t>(limit));

    std::vector<User> result;
    result.reserve(limit);
    while (stmt.executeStep())
        result.push_back(rowToUser(stmt));
    return result;
}

std::vector<User> UserRepository::findByIds(std::span<const utils::UserId> ids) const
{
    if (ids.empty())
        return {};

    // Собираем placeholders: "(?,?,?,...)". Дедупликацию делает сам SQL:
    // id — PRIMARY KEY, IN-выборка не задвоит строку при дубликатах во входе.
    std::string placeholders;
    placeholders.reserve(ids.size() * 2 + 2);
    placeholders.push_back('(');
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0)
            placeholders.push_back(',');
        placeholders.push_back('?');
    }
    placeholders.push_back(')');

    SQLite::Statement stmt(db_, std::string(SELECT_BY_IDS_PREFIX) + placeholders);
    int idx = 1;
    for (const auto id : ids)
        stmt.bind(idx++, static_cast<std::int64_t>(id));

    std::vector<User> result;
    result.reserve(ids.size());
    while (stmt.executeStep())
        result.push_back(rowToUser(stmt));
    return result;
}

void UserRepository::remove(const utils::UserId userId)
{
    SQLite::Statement stmt(db_, std::string(DELETE_BY_ID));
    stmt.bind(1, static_cast<std::int64_t>(userId));
    stmt.exec();
}

std::expected<void, AuthError> UserRepository::updateInfo(
    const utils::UserId userId,
    const std::string &username,
    const std::string &displayName,
    const std::time_t birthDate,
    const utils::Country country)
{
    SQLite::Statement stmt(db_, std::string(UPDATE_INFO));
    stmt.bind(1, username);
    stmt.bind(2, displayName);
    stmt.bind(3, countryToString(country));
    stmt.bind(4, static_cast<std::int64_t>(birthDate));
    stmt.bind(5, static_cast<std::int64_t>(userId));

    int changed = 0;
    try
    {
        changed = stmt.exec();
    }
    catch (const SQLite::Exception &e)
    {
        if (e.getExtendedErrorCode() == SQLITE_CONSTRAINT_UNIQUE)
            return std::unexpected(AuthError::UsernameTaken);
        throw;
    }

    if (changed == 0)
        return std::unexpected(AuthError::UserNotFound);
    return {};
}

std::expected<void, AuthError> UserRepository::updatePassword(
    const utils::UserId userId,
    const std::string &newPasswordHash)
{
    SQLite::Statement stmt(db_, std::string(UPDATE_PASSWORD));
    stmt.bind(1, newPasswordHash);
    stmt.bind(2, static_cast<std::int64_t>(userId));

    if (stmt.exec() == 0)
        return std::unexpected(AuthError::UserNotFound);
    return {};
}

} // namespace auth

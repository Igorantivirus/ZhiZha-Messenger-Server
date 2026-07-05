#include <AuthService/AuthService.hpp>

#include <utility>

namespace auth
{

AuthService::AuthService(UserRepository &userRepo,
                         TokenService &tokens,
                         utils::PasswordHasher &hasher,
                         utils::Validator &validator)
    : userRepo_(userRepo),
      tokens_(tokens),
      hasher_(hasher),
      validator_(validator)
{
}

std::expected<AuthSuccess, AuthError> AuthService::registrate(UserRegistrate reg)
{
    // Валидации: формат данных. birthDate сюда не закладывается — это уже
    // продуктовая политика (мин. возраст и т.п.), её добавим, когда появится.
    if (!validator_.isValidUsername(reg.username))
        return std::unexpected(AuthError::UsernameValidation);
    if (!validator_.isValidPassword(reg.password))
        return std::unexpected(AuthError::WeakPassword);
    if (!validator_.isValidDisplayName(reg.displayName))
        return std::unexpected(AuthError::UsernameValidation);

    // Уникальность username гарантируется UNIQUE на уровне репо.
    // Предварительной проверки нет — она оставляла race-окно между SELECT
    // и INSERT. Репо сам различает «занято» и сообщает доменно.
    const auto passwordHash = hasher_.hash(reg.password);
    auto created = userRepo_.create(
        reg.username,
        reg.displayName,
        passwordHash,
        reg.birthDate,
        reg.country);
    if (!created)
        return std::unexpected(created.error());

    return tokens_.issuePair(*created);
}

std::expected<AuthSuccess, AuthError> AuthService::login(Validations val)
{
    auto user = userRepo_.findUserByUsername(val.username);
    if (!user)
        return std::unexpected(AuthError::InvalidCredentials);

    if (!hasher_.verify(val.password, user->passwordHash))
        return std::unexpected(AuthError::InvalidCredentials);

    return tokens_.issuePair(user->id);
}

std::expected<AuthSuccess, AuthError> AuthService::refresh(std::string oldRefreshToken)
{
    return tokens_.refresh(oldRefreshToken);
}

void AuthService::logout(std::string refreshTocken)
{
    // Только refresh — access-токен этой сессии умрёт сам по истечении
    // короткого TTL. См. архитектурное решение #5.
    tokens_.revokeRefresh(refreshTocken);
}

void AuthService::logoutAll(utils::UserId userId)
{
    tokens_.revokeAllForUser(userId);
}

std::expected<utils::UserId, AuthError> AuthService::validateAccess(const std::string &accessToken)
{
    return tokens_.validateAccess(accessToken);
}

std::expected<void, AuthError> AuthService::updateUser(utils::UserId userId, UserUpdate upd)
{
    if (!validator_.isValidUsername(upd.username))
        return std::unexpected(AuthError::UsernameValidation);
    if (!validator_.isValidDisplayName(upd.displayName))
        return std::unexpected(AuthError::UsernameValidation);

    // Уникальность username проверяет репо (UNIQUE). Свой собственный
    // username не считается коллизией: SQL UPDATE с тем же значением
    // не нарушает UNIQUE, ошибки не будет.
    auto res = userRepo_.updateInfo(
        userId,
        upd.username,
        upd.displayName,
        upd.birthDate,
        upd.country);
    if (!res)
        return std::unexpected(res.error());

    return {};
}

std::expected<void, AuthError> AuthService::updatePassword(utils::UserId userId,
                                                            const std::string &oldPassword,
                                                            const std::string &newPassword)
{
    auto user = userRepo_.findUserById(userId);
    if (!user)
        return std::unexpected(AuthError::UserNotFound);

    if (!hasher_.verify(oldPassword, user->passwordHash))
        return std::unexpected(AuthError::InvalidCredentials);

    if (!validator_.isValidPassword(newPassword))
        return std::unexpected(AuthError::WeakPassword);

    const auto newHash = hasher_.hash(newPassword);
    auto res = userRepo_.updatePassword(userId, newHash);
    if (!res)
        return std::unexpected(res.error());

    // После смены пароля — все активные refresh-сессии этого пользователя
    // должны быть инвалидированы. Access-токены доживут до конца TTL —
    // это компромисс, см. решение #5.
    tokens_.revokeAllForUser(userId);

    return {};
}

} // namespace auth

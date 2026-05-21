#pragma once

#include <expected>
#include <optional>

#include <Protocol/Types.hpp>

#include "Interfaces/ICredentialsValidator.hpp"
#include "Interfaces/IPasswordHasher.hpp"
#include "Interfaces/ITokenService.hpp"
#include "Interfaces/IUserRepository.hpp"
#include "Types/AuthError.hpp"
#include "Types/AuthSuccess.hpp"

class AuthService
{
public:
    AuthService(IPasswordHasher &hasher, IUserRepository &users, ITokenService &tokens, ICredentialsValidator &validator)
        : hasher_(hasher),
          users_(users),
          tokens_(tokens),
          validator_(validator)
    {
    }

    // Регистрация
    std::expected<AuthSuccess, AuthError> registerUser(const std::string &username, const std::string &password, const std::string &displayName)
    {
        if (!validator_.isValidUsername(username))
            return std::unexpected(AuthError::UsernameValidation);
        if (!validator_.isValidPassword(password))
            return std::unexpected(AuthError::WeakPassword);
        if (users_.findUserByUsername(username).has_value())
            return std::unexpected(AuthError::UsernameTaken);

        std::string passwordHash = hasher_.hash(password);

        protocol::UserId newId = users_.create(username, displayName, passwordHash);

        return tokens_.issuePair(newId);
    }

    // Логин
    std::expected<AuthSuccess, AuthError> login(const std::string &username, const std::string &password)
    {
        std::optional<User> user = users_.findUserByUsername(username);
        if (!user.has_value())
        {
            // Холостая проверка хеша, чтобы время ответа не выдавало отсутствие юзера.
            hasher_.verify(password, getDummyHashForTimeing());
            return std::unexpected(AuthError::InvalidCredentials);
        }
        if (!hasher_.verify(password, user->passwordHash))
            return std::unexpected(AuthError::InvalidCredentials);

        return tokens_.issuePair(user->id);
    }

    // Обновление пары токенов
    std::expected<AuthSuccess, AuthError> refresh(const std::string &refreshToken)
    {
        std::optional<AuthSuccess> tokens = tokens_.refresh(refreshToken);
        if (!tokens.has_value())
            return std::unexpected(AuthError::InvalidToken);
        return tokens.value();
    }

    // Выход (отзыв конкретного refresh)
    void logout(const std::string &refreshToken)
    {
        return tokens_.revokeRefresh(refreshToken);
    }

    // Выход со всех устройств
    void logoutAll(protocol::UserId userId)
    {
        tokens_.revokeAllForUser(userId);
    }

    // Проверка access-токена
    std::optional<protocol::UserId> validateAccess(const std::string &accessToken)
    {
        return tokens_.validateAccess(accessToken);
    }

private:
    IPasswordHasher &hasher_;
    IUserRepository &users_;
    ITokenService &tokens_;
    ICredentialsValidator &validator_;

private:
    std::string getDummyHashForTimeing() const
    {
        return hasher_.hash("ABOBA228");
    }
};
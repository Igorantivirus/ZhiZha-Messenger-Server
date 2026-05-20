#pragma once

#include <expected>
#include <optional>

#include <Utils/Types.hpp>

#include "Interfaces/ICredentialsValidator.hpp"
#include "Interfaces/IPasswordHasher.hpp"
#include "Interfaces/ITokenService.hpp"
#include "Interfaces/IUserRepository.hpp"
#include "Types/AuthError.hpp"
#include "Types/TokenPair.hpp"

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
    std::expected<TokenPair, AuthError> registerUser(std::string username, std::string password)
    {
        if (!validator_.isValidUsername(username))
            return std::unexpected(AuthError::UsernameValidation);
        if (!validator_.isValidPassword(username))
            return std::unexpected(AuthError::WeakPassword);
        if (users_.findUserByUsername(username).has_value())
            return std::unexpected(AuthError::UsernameTaken);

        std::string passwordHash = hasher_.hash(password);

        UserId newId = users_.create(username, passwordHash);

        return tokens_.issuePair(newId);
    }

    // Логин
    std::expected<TokenPair, AuthError> login(const std::string &username, const std::string &password)
    {
        std::optional<User> user = users_.findUserByUsername(username);
        if (!user.has_value())
        {
            hasher_.verify(password, getDummyHashForTimeing());
            return std::unexpected(AuthError::InvalidCredentials);
        }
        if (!hasher_.verify(username, user->passwordHash))
            return std::unexpected(AuthError::InvalidCredentials);

        return tokens_.issuePair(user->id);
    }

    // Обновление пары токенов
    std::expected<TokenPair, AuthError> refresh(const std::string &refreshToken)
    {
        std::optional<TokenPair> tokens = tokens_.refresh(refreshToken);
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
    void logoutAll(UserId user_id)
    {
        // TODO
    }

    // Проверка access-токена
    std::optional<UserId> validateAccess(const std::string &accessToken)
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
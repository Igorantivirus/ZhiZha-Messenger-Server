#pragma once

#include <expected>
#include <string>

#include <Utils/PasswordHasher.hpp>
#include <Utils/Types.hpp>
#include <Utils/Validator.hpp>

#include "Subservices/TokenService.hpp"
#include "Subservices/UserRepository.hpp"
#include "Types/AuthError.hpp"
#include "Types/AuthSuccess.hpp"
#include "Types/UserRegistrate.hpp"
#include "Types/UserUpdate.hpp"
#include "Types/Validations.hpp"

namespace auth
{

// Command-сервис auth-домена. Только мутации и валидации.
// Чтения профиля/поиск пользователей — в UserQueryService, он работает
// напрямую с тем же UserRepository.
class AuthService
{
public:
    AuthService(UserRepository &userRepo,
                TokenService &tokens,
                utils::PasswordHasher &hasher,
                utils::Validator &validator);

    std::expected<AuthSuccess, AuthError> registrate(UserRegistrate reg);       // Регистрация
    std::expected<AuthSuccess, AuthError> login(Validations val);               // Валидация
    std::expected<AuthSuccess, AuthError> refresh(std::string oldRefreshToken); // Рефреш

    void logout(std::string refreshTocken); // Завершить сессию
    void logoutAll(utils::UserId);          // Завершить все сесии по id

    std::expected<utils::UserId, AuthError> validateAccess(const std::string &accessToken);

    // Обновление редактируемых полей профиля. userId передаётся отдельно
    // (берётся транспортом из access-токена), UserUpdate — это «что менять».
    std::expected<void, AuthError> updateUser(utils::UserId userId, UserUpdate upd);

    // Смена пароля. Требует подтверждения старым паролем; после успешной
    // смены — отзыв всех refresh-токенов пользователя (логаут со всех устройств).
    std::expected<void, AuthError> updatePassword(utils::UserId userId,
                                                  const std::string &oldPassword,
                                                  const std::string &newPassword);

private:
    UserRepository &userRepo_;
    TokenService &tokens_;
    utils::PasswordHasher &hasher_;
    utils::Validator &validator_;
};

} // namespace auth

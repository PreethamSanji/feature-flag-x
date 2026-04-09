#pragma once

#include "models/user.h"
#include "repositories/connection_pool.h"
#include <expected>
#include <optional>
#include <string>

namespace ffx {

/// Authentication service for JWT tokens, password hashing, and API keys.
class AuthService {
public:
    AuthService(ConnectionPool& pool, const std::string& jwt_secret, int jwt_expiry_seconds);

    /// Register a new user (admin-only operation).
    std::expected<User, std::string> register_user(
        const std::string& email, const std::string& password, UserRole role);

    /// Login and return a JWT token.
    std::expected<std::string, std::string> login(
        const std::string& email, const std::string& password);

    /// Validate a JWT token and return the user.
    std::expected<User, std::string> validate_token(const std::string& token);

    /// Validate an API key and return the user.
    std::expected<User, std::string> validate_api_key(const std::string& api_key);

private:
    /// Hash a password with the given salt (SHA-256, demo only).
    std::string hash_password(const std::string& password, const std::string& salt);

    /// Generate a random hex string of the given byte length.
    std::string generate_random_hex(int bytes);

    /// Generate an API key.
    std::string generate_api_key();

    /// Find a user by email.
    std::expected<std::optional<User>, std::string> find_by_email(const std::string& email);

    ConnectionPool& pool_;
    std::string jwt_secret_;
    int jwt_expiry_seconds_;
};

} // namespace ffx

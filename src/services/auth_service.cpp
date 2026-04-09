#include "services/auth_service.h"
#include <jwt-cpp/jwt.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <format>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ffx {

AuthService::AuthService(ConnectionPool& pool, const std::string& jwt_secret, int jwt_expiry_seconds)
    : pool_(pool), jwt_secret_(jwt_secret), jwt_expiry_seconds_(jwt_expiry_seconds) {}

std::string AuthService::hash_password(const std::string& password, const std::string& salt) {
    std::string input = salt + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string AuthService::generate_random_hex(int bytes) {
    std::vector<unsigned char> buf(bytes);
    RAND_bytes(buf.data(), bytes);

    std::ostringstream oss;
    for (auto b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

std::string AuthService::generate_api_key() {
    return "ffx_ak_" + generate_random_hex(28);
}

std::expected<std::optional<User>, std::string> AuthService::find_by_email(const std::string& email) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "SELECT id, email, hashed_password, salt, role, api_key, created_at "
            "FROM users WHERE email = $1", email);

        txn.commit();

        if (result.empty()) {
            return std::optional<User>(std::nullopt);
        }

        User u;
        u.id              = result[0]["id"].as<std::string>();
        u.email           = result[0]["email"].as<std::string>();
        u.hashed_password = result[0]["hashed_password"].as<std::string>();
        u.salt            = result[0]["salt"].as<std::string>();
        u.role            = role_from_string(result[0]["role"].as<std::string>());
        u.api_key         = result[0]["api_key"].as<std::string>();
        u.created_at      = result[0]["created_at"].as<std::string>();
        return std::optional<User>(std::move(u));
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<User, std::string> AuthService::register_user(
    const std::string& email, const std::string& password, UserRole role) {
    try {
        auto salt = generate_random_hex(16);
        auto hashed = hash_password(password, salt);
        auto api_key = generate_api_key();

        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "INSERT INTO users (email, hashed_password, salt, role, api_key) "
            "VALUES ($1, $2, $3, $4::user_role, $5) RETURNING id, email, role, api_key, created_at",
            email, hashed, salt, role_to_string(role), api_key);

        txn.commit();

        if (result.empty()) {
            return std::unexpected(std::string("Insert returned no rows"));
        }

        User u;
        u.id         = result[0]["id"].as<std::string>();
        u.email      = result[0]["email"].as<std::string>();
        u.role       = role_from_string(result[0]["role"].as<std::string>());
        u.api_key    = result[0]["api_key"].as<std::string>();
        u.created_at = result[0]["created_at"].as<std::string>();
        return u;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to register user: {}", e.what()));
    }
}

std::expected<std::string, std::string> AuthService::login(
    const std::string& email, const std::string& password) {

    auto user_result = find_by_email(email);
    if (!user_result.has_value()) return std::unexpected(user_result.error());
    if (!user_result->has_value()) return std::unexpected(std::string("Invalid credentials"));

    auto& user = user_result->value();
    auto hashed = hash_password(password, user.salt);

    if (hashed != user.hashed_password) {
        return std::unexpected(std::string("Invalid credentials"));
    }

    auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
        .set_issuer("featureflagx")
        .set_subject(user.id)
        .set_payload_claim("email", jwt::claim(user.email))
        .set_payload_claim("role", jwt::claim(role_to_string(user.role)))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds(jwt_expiry_seconds_))
        .sign(jwt::algorithm::hs256{jwt_secret_});

    return token;
}

std::expected<User, std::string> AuthService::validate_token(const std::string& token) {
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwt_secret_})
            .with_issuer("featureflagx");

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        User u;
        u.id    = decoded.get_subject();
        u.email = decoded.get_payload_claim("email").as_string();
        u.role  = role_from_string(decoded.get_payload_claim("role").as_string());
        return u;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Invalid token: {}", e.what()));
    }
}

std::expected<User, std::string> AuthService::validate_api_key(const std::string& api_key) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "SELECT id, email, role, api_key, created_at FROM users WHERE api_key = $1",
            api_key);

        txn.commit();

        if (result.empty()) {
            return std::unexpected(std::string("Invalid API key"));
        }

        User u;
        u.id         = result[0]["id"].as<std::string>();
        u.email      = result[0]["email"].as<std::string>();
        u.role       = role_from_string(result[0]["role"].as<std::string>());
        u.api_key    = result[0]["api_key"].as<std::string>();
        u.created_at = result[0]["created_at"].as<std::string>();
        return u;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("API key validation failed: {}", e.what()));
    }
}

} // namespace ffx

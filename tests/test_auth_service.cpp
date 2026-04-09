#include <gtest/gtest.h>
#include "models/user.h"
#include <jwt-cpp/jwt.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace ffx {
namespace {

// --- Role permission tests ---

class RoleTest : public ::testing::Test {};

TEST_F(RoleTest, RoleConversion) {
    EXPECT_EQ(role_to_string(UserRole::viewer), "viewer");
    EXPECT_EQ(role_to_string(UserRole::editor), "editor");
    EXPECT_EQ(role_to_string(UserRole::admin), "admin");

    EXPECT_EQ(role_from_string("viewer"), UserRole::viewer);
    EXPECT_EQ(role_from_string("editor"), UserRole::editor);
    EXPECT_EQ(role_from_string("admin"), UserRole::admin);

    EXPECT_THROW(role_from_string("invalid"), std::invalid_argument);
}

TEST_F(RoleTest, AdminHasAllPermissions) {
    EXPECT_TRUE(role_has_permission(UserRole::admin, UserRole::viewer));
    EXPECT_TRUE(role_has_permission(UserRole::admin, UserRole::editor));
    EXPECT_TRUE(role_has_permission(UserRole::admin, UserRole::admin));
}

TEST_F(RoleTest, EditorHasEditorAndViewerPermissions) {
    EXPECT_TRUE(role_has_permission(UserRole::editor, UserRole::viewer));
    EXPECT_TRUE(role_has_permission(UserRole::editor, UserRole::editor));
    EXPECT_FALSE(role_has_permission(UserRole::editor, UserRole::admin));
}

TEST_F(RoleTest, ViewerOnlyHasViewerPermission) {
    EXPECT_TRUE(role_has_permission(UserRole::viewer, UserRole::viewer));
    EXPECT_FALSE(role_has_permission(UserRole::viewer, UserRole::editor));
    EXPECT_FALSE(role_has_permission(UserRole::viewer, UserRole::admin));
}

// --- JWT tests (testing jwt-cpp directly, same logic as AuthService) ---

class JwtTest : public ::testing::Test {
protected:
    const std::string secret_ = "test-secret-key-for-jwt-testing";
};

TEST_F(JwtTest, CreateAndVerifyToken) {
    auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
        .set_issuer("featureflagx")
        .set_subject("user-123")
        .set_payload_claim("email", jwt::claim(std::string("test@example.com")))
        .set_payload_claim("role", jwt::claim(std::string("admin")))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(1))
        .sign(jwt::algorithm::hs256{secret_});

    EXPECT_FALSE(token.empty());

    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{secret_})
        .with_issuer("featureflagx");

    auto decoded = jwt::decode(token);
    EXPECT_NO_THROW(verifier.verify(decoded));

    EXPECT_EQ(decoded.get_subject(), "user-123");
    EXPECT_EQ(decoded.get_payload_claim("email").as_string(), "test@example.com");
    EXPECT_EQ(decoded.get_payload_claim("role").as_string(), "admin");
}

TEST_F(JwtTest, ExpiredTokenRejected) {
    auto past = std::chrono::system_clock::now() - std::chrono::hours(2);
    auto token = jwt::create()
        .set_issuer("featureflagx")
        .set_subject("user-123")
        .set_issued_at(past)
        .set_expires_at(past + std::chrono::hours(1))
        .sign(jwt::algorithm::hs256{secret_});

    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{secret_})
        .with_issuer("featureflagx");

    auto decoded = jwt::decode(token);
    EXPECT_THROW(verifier.verify(decoded), jwt::error::token_verification_error);
}

TEST_F(JwtTest, WrongSecretRejected) {
    auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
        .set_issuer("featureflagx")
        .set_subject("user-123")
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(1))
        .sign(jwt::algorithm::hs256{secret_});

    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{"wrong-secret"})
        .with_issuer("featureflagx");

    auto decoded = jwt::decode(token);
    EXPECT_THROW(verifier.verify(decoded), jwt::error::token_verification_error);
}

TEST_F(JwtTest, InvalidTokenThrows) {
    EXPECT_THROW(jwt::decode("not.a.valid.token.at.all"), std::exception);
}

// --- Password hashing tests ---

class PasswordHashTest : public ::testing::Test {
protected:
    std::string hash_sha256(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);

        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return oss.str();
    }
};

TEST_F(PasswordHashTest, SameInputProducesSameHash) {
    auto hash1 = hash_sha256("salt123password");
    auto hash2 = hash_sha256("salt123password");
    EXPECT_EQ(hash1, hash2);
}

TEST_F(PasswordHashTest, DifferentSaltProducesDifferentHash) {
    auto hash1 = hash_sha256("salt_a" + std::string("password"));
    auto hash2 = hash_sha256("salt_b" + std::string("password"));
    EXPECT_NE(hash1, hash2);
}

TEST_F(PasswordHashTest, HashIs64HexChars) {
    auto hash = hash_sha256("test");
    EXPECT_EQ(hash.length(), 64u);

    // All characters should be hex
    for (char c : hash) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character: " << c;
    }
}

// --- User model tests ---

TEST(UserModelTest, JsonSerialization) {
    User u;
    u.id = "user-uuid";
    u.email = "test@example.com";
    u.role = UserRole::editor;
    u.api_key = "ffx_ak_test";
    u.created_at = "2026-01-01T00:00:00Z";

    nlohmann::json j = u;
    EXPECT_EQ(j["email"], "test@example.com");
    EXPECT_EQ(j["role"], "editor");
    EXPECT_EQ(j["api_key"], "ffx_ak_test");

    // Password should NOT be in JSON output
    EXPECT_FALSE(j.contains("hashed_password"));
    EXPECT_FALSE(j.contains("salt"));
}

} // anonymous namespace
} // namespace ffx

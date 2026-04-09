#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace ffx {

enum class UserRole {
    viewer,
    editor,
    admin
};

/// Convert UserRole to/from string.
std::string role_to_string(UserRole r);
UserRole role_from_string(const std::string& s);

/// Check if actual role meets the required permission level.
bool role_has_permission(UserRole actual, UserRole required);

/// User entity.
struct User {
    std::string id;
    std::string email;
    std::string hashed_password;
    std::string salt;
    UserRole role = UserRole::viewer;
    std::string api_key;
    std::string created_at;
};

void to_json(nlohmann::json& j, const User& u);
void from_json(const nlohmann::json& j, User& u);

} // namespace ffx

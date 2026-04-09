#include "models/user.h"
#include <stdexcept>

namespace ffx {

std::string role_to_string(UserRole r) {
    switch (r) {
        case UserRole::viewer: return "viewer";
        case UserRole::editor: return "editor";
        case UserRole::admin:  return "admin";
    }
    return "viewer";
}

UserRole role_from_string(const std::string& s) {
    if (s == "viewer") return UserRole::viewer;
    if (s == "editor") return UserRole::editor;
    if (s == "admin")  return UserRole::admin;
    throw std::invalid_argument("Unknown role: " + s);
}

bool role_has_permission(UserRole actual, UserRole required) {
    // admin > editor > viewer
    return static_cast<int>(actual) >= static_cast<int>(required);
}

void to_json(nlohmann::json& j, const User& u) {
    j = nlohmann::json{
        {"id", u.id},
        {"email", u.email},
        {"role", role_to_string(u.role)},
        {"api_key", u.api_key},
        {"created_at", u.created_at}
    };
}

void from_json(const nlohmann::json& j, User& u) {
    if (j.contains("id"))    j.at("id").get_to(u.id);
    j.at("email").get_to(u.email);
    if (j.contains("role"))  u.role = role_from_string(j.at("role").get<std::string>());
    if (j.contains("api_key")) j.at("api_key").get_to(u.api_key);
    if (j.contains("created_at")) j.at("created_at").get_to(u.created_at);
}

} // namespace ffx

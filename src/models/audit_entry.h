#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace ffx {

enum class AuditAction {
    create,
    update,
    del,
    toggle
};

/// Convert AuditAction to/from string.
std::string audit_action_to_string(AuditAction a);
AuditAction audit_action_from_string(const std::string& s);

/// Audit log entry.
struct AuditEntry {
    int64_t id = 0;
    std::string flag_id;
    std::string user_id;
    AuditAction action = AuditAction::create;
    nlohmann::json prev_value;
    nlohmann::json new_value;
    std::string timestamp;
};

void to_json(nlohmann::json& j, const AuditEntry& e);

} // namespace ffx

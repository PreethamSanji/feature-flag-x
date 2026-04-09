#include "models/audit_entry.h"
#include <stdexcept>

namespace ffx {

std::string audit_action_to_string(AuditAction a) {
    switch (a) {
        case AuditAction::create: return "create";
        case AuditAction::update: return "update";
        case AuditAction::del:    return "delete";
        case AuditAction::toggle: return "toggle";
    }
    return "create";
}

AuditAction audit_action_from_string(const std::string& s) {
    if (s == "create") return AuditAction::create;
    if (s == "update") return AuditAction::update;
    if (s == "delete") return AuditAction::del;
    if (s == "toggle") return AuditAction::toggle;
    throw std::invalid_argument("Unknown audit action: " + s);
}

void to_json(nlohmann::json& j, const AuditEntry& e) {
    j = nlohmann::json{
        {"id", e.id},
        {"flag_id", e.flag_id},
        {"user_id", e.user_id},
        {"action", audit_action_to_string(e.action)},
        {"prev_value", e.prev_value},
        {"new_value", e.new_value},
        {"timestamp", e.timestamp}
    };
}

} // namespace ffx

#include "repositories/audit_repo.h"
#include <format>

namespace ffx {

AuditRepo::AuditRepo(ConnectionPool& pool) : pool_(pool) {}

std::expected<void, std::string> AuditRepo::log_action(const AuditEntry& entry) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        txn.exec_params(
            "INSERT INTO audit_log (flag_id, user_id, action, prev_value, new_value) "
            "VALUES ($1::uuid, $2::uuid, $3::audit_action, $4::jsonb, $5::jsonb)",
            entry.flag_id,
            entry.user_id,
            audit_action_to_string(entry.action),
            entry.prev_value.dump(),
            entry.new_value.dump());

        txn.commit();
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to log audit action: {}", e.what()));
    }
}

std::expected<std::vector<AuditEntry>, std::string> AuditRepo::find_by_flag_key(
    const std::string& flag_key, int limit) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "SELECT al.id, al.flag_id, al.user_id, al.action, "
            "al.prev_value, al.new_value, al.timestamp "
            "FROM audit_log al "
            "JOIN flags f ON al.flag_id = f.id "
            "WHERE f.key = $1 "
            "ORDER BY al.timestamp DESC LIMIT $2",
            flag_key, limit);

        std::vector<AuditEntry> entries;
        entries.reserve(result.size());
        for (const auto& row : result) {
            AuditEntry e;
            e.id         = row["id"].as<int64_t>();
            e.flag_id    = row["flag_id"].as<std::string>();
            e.user_id    = row["user_id"].as<std::string>();
            e.action     = audit_action_from_string(row["action"].as<std::string>());
            e.prev_value = nlohmann::json::parse(
                row["prev_value"].is_null() ? "null" : row["prev_value"].as<std::string>());
            e.new_value  = nlohmann::json::parse(
                row["new_value"].is_null() ? "null" : row["new_value"].as<std::string>());
            e.timestamp  = row["timestamp"].as<std::string>();
            entries.push_back(std::move(e));
        }

        txn.commit();
        return entries;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to find audit entries: {}", e.what()));
    }
}

} // namespace ffx

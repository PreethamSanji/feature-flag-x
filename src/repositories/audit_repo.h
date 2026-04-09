#pragma once

#include "models/audit_entry.h"
#include "repositories/connection_pool.h"
#include <expected>
#include <vector>
#include <string>

namespace ffx {

/// Repository for audit log operations.
class AuditRepo {
public:
    explicit AuditRepo(ConnectionPool& pool);

    /// Log an audit action.
    std::expected<void, std::string> log_action(const AuditEntry& entry);

    /// Find audit entries by flag key, with optional limit.
    std::expected<std::vector<AuditEntry>, std::string> find_by_flag_key(
        const std::string& flag_key, int limit = 50);

private:
    ConnectionPool& pool_;
};

} // namespace ffx

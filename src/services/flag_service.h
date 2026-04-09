#pragma once

#include "models/flag.h"
#include "repositories/flag_repo.h"
#include "repositories/audit_repo.h"
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include <unordered_map>

namespace ffx {

/// Service layer for feature flag operations and evaluation.
class FlagService {
public:
    FlagService(FlagRepo& flag_repo, AuditRepo& audit_repo);

    /// List all flags, optionally filtered by environment.
    std::expected<std::vector<Flag>, std::string> list_flags(const std::string& environment = "");

    /// Get a single flag by key and environment.
    std::expected<std::optional<Flag>, std::string> get_flag(
        const std::string& key, const std::string& environment = "production");

    /// Create a new flag.
    std::expected<Flag, std::string> create_flag(const Flag& flag, const std::string& user_id);

    /// Update an existing flag.
    std::expected<Flag, std::string> update_flag(
        const std::string& key, const std::string& environment,
        const Flag& flag, const std::string& user_id);

    /// Delete a flag.
    std::expected<void, std::string> delete_flag(
        const std::string& key, const std::string& environment, const std::string& user_id);

    /// Toggle a flag's enabled state.
    std::expected<Flag, std::string> toggle_flag(
        const std::string& key, const std::string& environment, const std::string& user_id);

    /// Evaluate a flag against a context.
    std::expected<EvaluationResult, std::string> evaluate(
        const std::string& flag_key,
        const std::unordered_map<std::string, std::string>& context,
        const std::string& environment = "production");

    /// Evaluate multiple flags in bulk.
    std::expected<std::vector<EvaluationResult>, std::string> evaluate_bulk(
        const std::vector<std::string>& flag_keys,
        const std::unordered_map<std::string, std::string>& context,
        const std::string& environment = "production");

private:
    /// Evaluate targeting rules against a context (first-match-wins).
    EvaluationResult evaluate_flag(
        const Flag& flag,
        const std::unordered_map<std::string, std::string>& context);

    /// Check if a single rule matches the context.
    bool rule_matches(
        const TargetingRule& rule,
        const std::unordered_map<std::string, std::string>& context,
        const std::string& flag_key);

    /// Deterministic percentage hash for rollout.
    int percentage_hash(const std::string& key, const std::string& value);

    FlagRepo& flag_repo_;
    AuditRepo& audit_repo_;
};

} // namespace ffx

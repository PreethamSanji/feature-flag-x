#include "services/flag_service.h"
#include <functional>
#include <algorithm>

namespace ffx {

FlagService::FlagService(FlagRepo& flag_repo, AuditRepo& audit_repo)
    : flag_repo_(flag_repo), audit_repo_(audit_repo) {}

std::expected<std::vector<Flag>, std::string> FlagService::list_flags(const std::string& environment) {
    return flag_repo_.list(environment);
}

std::expected<std::optional<Flag>, std::string> FlagService::get_flag(
    const std::string& key, const std::string& environment) {
    return flag_repo_.find_by_key(key, environment);
}

std::expected<Flag, std::string> FlagService::create_flag(const Flag& flag, const std::string& user_id) {
    auto result = flag_repo_.create(flag);
    if (result.has_value()) {
        AuditEntry entry;
        entry.flag_id = result->id;
        entry.user_id = user_id;
        entry.action = AuditAction::create;
        entry.new_value = result.value();
        audit_repo_.log_action(entry);
    }
    return result;
}

std::expected<Flag, std::string> FlagService::update_flag(
    const std::string& key, const std::string& environment,
    const Flag& flag, const std::string& user_id) {
    auto existing = flag_repo_.find_by_key(key, environment);
    if (!existing.has_value()) return std::unexpected(existing.error());
    if (!existing->has_value()) return std::unexpected(std::string("Flag not found"));

    auto result = flag_repo_.update(key, environment, flag);
    if (result.has_value()) {
        AuditEntry entry;
        entry.flag_id = result->id;
        entry.user_id = user_id;
        entry.action = AuditAction::update;
        entry.prev_value = existing->value();
        entry.new_value = result.value();
        audit_repo_.log_action(entry);
    }
    return result;
}

std::expected<void, std::string> FlagService::delete_flag(
    const std::string& key, const std::string& environment, const std::string& user_id) {
    auto existing = flag_repo_.find_by_key(key, environment);
    if (!existing.has_value()) return std::unexpected(existing.error());
    if (!existing->has_value()) return std::unexpected(std::string("Flag not found"));

    auto& flag = existing->value();
    AuditEntry entry;
    entry.flag_id = flag.id;
    entry.user_id = user_id;
    entry.action = AuditAction::del;
    entry.prev_value = flag;

    auto result = flag_repo_.remove(key, environment);
    if (result.has_value()) {
        audit_repo_.log_action(entry);
    }
    return result;
}

std::expected<Flag, std::string> FlagService::toggle_flag(
    const std::string& key, const std::string& environment, const std::string& user_id) {
    auto existing = flag_repo_.find_by_key(key, environment);
    if (!existing.has_value()) return std::unexpected(existing.error());
    if (!existing->has_value()) return std::unexpected(std::string("Flag not found"));

    auto result = flag_repo_.toggle(key, environment);
    if (result.has_value()) {
        AuditEntry entry;
        entry.flag_id = result->id;
        entry.user_id = user_id;
        entry.action = AuditAction::toggle;
        entry.prev_value = existing->value();
        entry.new_value = result.value();
        audit_repo_.log_action(entry);
    }
    return result;
}

std::expected<EvaluationResult, std::string> FlagService::evaluate(
    const std::string& flag_key,
    const std::unordered_map<std::string, std::string>& context,
    const std::string& environment) {

    auto result = flag_repo_.find_by_key(flag_key, environment);
    if (!result.has_value()) return std::unexpected(result.error());
    if (!result->has_value()) {
        return std::unexpected(std::string("Flag not found: " + flag_key));
    }

    return evaluate_flag(result->value(), context);
}

std::expected<std::vector<EvaluationResult>, std::string> FlagService::evaluate_bulk(
    const std::vector<std::string>& flag_keys,
    const std::unordered_map<std::string, std::string>& context,
    const std::string& environment) {

    std::vector<EvaluationResult> results;
    results.reserve(flag_keys.size());

    for (const auto& key : flag_keys) {
        auto result = evaluate(key, context, environment);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        results.push_back(std::move(result.value()));
    }
    return results;
}

EvaluationResult FlagService::evaluate_flag(
    const Flag& flag,
    const std::unordered_map<std::string, std::string>& context) {

    EvaluationResult result;
    result.flag_key = flag.key;
    result.version = flag.version;

    // Disabled flag returns default
    if (!flag.enabled) {
        result.value = flag.default_value;
        result.reason = "disabled";
        return result;
    }

    // Evaluate rules (first-match-wins)
    for (const auto& rule : flag.rules) {
        if (rule_matches(rule, context, flag.key)) {
            result.value = rule.value;
            result.reason = "rule_match";
            return result;
        }
    }

    // No rule matched — return default
    result.value = flag.default_value;
    result.reason = "default";
    return result;
}

bool FlagService::rule_matches(
    const TargetingRule& rule,
    const std::unordered_map<std::string, std::string>& context,
    const std::string& flag_key) {

    // Percentage operator uses deterministic hash
    if (rule.op == "percentage") {
        auto it = context.find(rule.attribute);
        if (it == context.end()) return false;

        int hash = percentage_hash(flag_key, it->second);
        return hash < rule.rollout_percentage;
    }

    // All other operators require the attribute to exist in context
    auto it = context.find(rule.attribute);
    if (it == context.end()) return false;

    const std::string& ctx_value = it->second;

    if (rule.op == "in") {
        return std::find(rule.values.begin(), rule.values.end(), ctx_value) != rule.values.end();
    }

    if (rule.op == "not_in") {
        return std::find(rule.values.begin(), rule.values.end(), ctx_value) == rule.values.end();
    }

    if (rule.op == "eq") {
        return !rule.values.empty() && ctx_value == rule.values[0];
    }

    if (rule.op == "neq") {
        return !rule.values.empty() && ctx_value != rule.values[0];
    }

    if (rule.op == "gt") {
        if (rule.values.empty()) return false;
        try {
            return std::stod(ctx_value) > std::stod(rule.values[0]);
        } catch (...) { return false; }
    }

    if (rule.op == "lt") {
        if (rule.values.empty()) return false;
        try {
            return std::stod(ctx_value) < std::stod(rule.values[0]);
        } catch (...) { return false; }
    }

    if (rule.op == "contains") {
        return !rule.values.empty() && ctx_value.find(rule.values[0]) != std::string::npos;
    }

    return false;
}

int FlagService::percentage_hash(const std::string& key, const std::string& value) {
    // Deterministic hash: hash(flag_key + ":" + attribute_value) % 100
    std::string input = key + ":" + value;
    std::size_t hash = std::hash<std::string>{}(input);
    return static_cast<int>(hash % 100);
}

} // namespace ffx

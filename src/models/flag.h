#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace ffx {

enum class FlagType {
    boolean_type,
    string_type,
    number_type,
    json_type
};

/// Convert FlagType to/from string for DB and JSON.
std::string flag_type_to_string(FlagType t);
FlagType flag_type_from_string(const std::string& s);

/// A single targeting rule within a feature flag.
struct TargetingRule {
    std::string attribute;
    std::string op;
    std::vector<std::string> values;
    int rollout_percentage = 100;
    nlohmann::json value;
};

void to_json(nlohmann::json& j, const TargetingRule& r);
void from_json(const nlohmann::json& j, TargetingRule& r);

/// Feature flag entity.
struct Flag {
    std::string id;
    std::string key;
    std::string description;
    FlagType flag_type = FlagType::boolean_type;
    nlohmann::json default_value = false;
    std::vector<TargetingRule> rules;
    bool enabled = true;
    int version = 1;
    std::string environment = "production";
    std::string created_at;
    std::string updated_at;
};

void to_json(nlohmann::json& j, const Flag& f);
void from_json(const nlohmann::json& j, Flag& f);

/// Result of evaluating a flag against a context.
struct EvaluationResult {
    std::string flag_key;
    nlohmann::json value;
    int version = 0;
    std::string reason;
};

void to_json(nlohmann::json& j, const EvaluationResult& r);

} // namespace ffx

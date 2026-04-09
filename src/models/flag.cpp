#include "models/flag.h"
#include <stdexcept>

namespace ffx {

std::string flag_type_to_string(FlagType t) {
    switch (t) {
        case FlagType::boolean_type: return "boolean";
        case FlagType::string_type:  return "string";
        case FlagType::number_type:  return "number";
        case FlagType::json_type:    return "json";
    }
    return "boolean";
}

FlagType flag_type_from_string(const std::string& s) {
    if (s == "boolean") return FlagType::boolean_type;
    if (s == "string")  return FlagType::string_type;
    if (s == "number")  return FlagType::number_type;
    if (s == "json")    return FlagType::json_type;
    throw std::invalid_argument("Unknown flag type: " + s);
}

void to_json(nlohmann::json& j, const TargetingRule& r) {
    j = nlohmann::json{
        {"attribute", r.attribute},
        {"operator", r.op},
        {"values", r.values},
        {"rollout_percentage", r.rollout_percentage},
        {"value", r.value}
    };
}

void from_json(const nlohmann::json& j, TargetingRule& r) {
    j.at("attribute").get_to(r.attribute);
    j.at("operator").get_to(r.op);
    j.at("values").get_to(r.values);
    j.at("rollout_percentage").get_to(r.rollout_percentage);
    r.value = j.at("value");
}

void to_json(nlohmann::json& j, const Flag& f) {
    j = nlohmann::json{
        {"id", f.id},
        {"key", f.key},
        {"description", f.description},
        {"flag_type", flag_type_to_string(f.flag_type)},
        {"default_value", f.default_value},
        {"rules", f.rules},
        {"enabled", f.enabled},
        {"version", f.version},
        {"environment", f.environment},
        {"created_at", f.created_at},
        {"updated_at", f.updated_at}
    };
}

void from_json(const nlohmann::json& j, Flag& f) {
    j.at("key").get_to(f.key);
    if (j.contains("id"))          j.at("id").get_to(f.id);
    if (j.contains("description")) j.at("description").get_to(f.description);
    if (j.contains("flag_type"))   f.flag_type = flag_type_from_string(j.at("flag_type").get<std::string>());
    if (j.contains("default_value")) f.default_value = j.at("default_value");
    if (j.contains("rules"))       j.at("rules").get_to(f.rules);
    if (j.contains("enabled"))     j.at("enabled").get_to(f.enabled);
    if (j.contains("version"))     j.at("version").get_to(f.version);
    if (j.contains("environment")) j.at("environment").get_to(f.environment);
    if (j.contains("created_at"))  j.at("created_at").get_to(f.created_at);
    if (j.contains("updated_at"))  j.at("updated_at").get_to(f.updated_at);
}

void to_json(nlohmann::json& j, const EvaluationResult& r) {
    j = nlohmann::json{
        {"flag_key", r.flag_key},
        {"value", r.value},
        {"version", r.version},
        {"reason", r.reason}
    };
}

} // namespace ffx

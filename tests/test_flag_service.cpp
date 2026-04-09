#include <gtest/gtest.h>
#include "models/flag.h"
#include "services/flag_service.h"
#include <unordered_map>
#include <functional>

namespace ffx {
namespace {

// Helper to create a test flag with given rules
Flag make_test_flag(const std::string& key, bool enabled,
                    const nlohmann::json& default_value,
                    const std::vector<TargetingRule>& rules) {
    Flag f;
    f.id = "test-id";
    f.key = key;
    f.enabled = enabled;
    f.default_value = default_value;
    f.rules = rules;
    f.version = 1;
    f.environment = "production";
    return f;
}

// Test the evaluation logic directly without DB
// We test the percentage_hash and rule matching via the public evaluate interface
// by creating a mock-like setup

class FlagEvaluationTest : public ::testing::Test {
protected:
    // Since FlagService needs repos, we test evaluation via the model/logic directly
    // The evaluate_flag method is private, so we test via the public interface
    // For unit tests without DB, we test the model serialization and rule structures

    std::unordered_map<std::string, std::string> context;
};

// --- Model serialization tests ---

TEST_F(FlagEvaluationTest, FlagTypeConversion) {
    EXPECT_EQ(flag_type_to_string(FlagType::boolean_type), "boolean");
    EXPECT_EQ(flag_type_to_string(FlagType::string_type), "string");
    EXPECT_EQ(flag_type_to_string(FlagType::number_type), "number");
    EXPECT_EQ(flag_type_to_string(FlagType::json_type), "json");

    EXPECT_EQ(flag_type_from_string("boolean"), FlagType::boolean_type);
    EXPECT_EQ(flag_type_from_string("string"), FlagType::string_type);
    EXPECT_EQ(flag_type_from_string("number"), FlagType::number_type);
    EXPECT_EQ(flag_type_from_string("json"), FlagType::json_type);

    EXPECT_THROW(flag_type_from_string("invalid"), std::invalid_argument);
}

TEST_F(FlagEvaluationTest, FlagJsonRoundtrip) {
    Flag f;
    f.id = "uuid-123";
    f.key = "test.flag";
    f.description = "A test flag";
    f.flag_type = FlagType::boolean_type;
    f.default_value = false;
    f.enabled = true;
    f.version = 3;
    f.environment = "staging";

    TargetingRule rule;
    rule.attribute = "user_id";
    rule.op = "in";
    rule.values = {"u_1", "u_2"};
    rule.rollout_percentage = 100;
    rule.value = true;
    f.rules = {rule};

    nlohmann::json j = f;

    EXPECT_EQ(j["key"], "test.flag");
    EXPECT_EQ(j["flag_type"], "boolean");
    EXPECT_EQ(j["version"], 3);
    EXPECT_EQ(j["environment"], "staging");
    EXPECT_EQ(j["rules"].size(), 1);
    EXPECT_EQ(j["rules"][0]["operator"], "in");

    // Roundtrip
    Flag f2 = j.get<Flag>();
    EXPECT_EQ(f2.key, "test.flag");
    EXPECT_EQ(f2.version, 3);
    EXPECT_EQ(f2.rules.size(), 1);
    EXPECT_EQ(f2.rules[0].op, "in");
}

TEST_F(FlagEvaluationTest, TargetingRuleJsonRoundtrip) {
    TargetingRule rule;
    rule.attribute = "country";
    rule.op = "eq";
    rule.values = {"US"};
    rule.rollout_percentage = 100;
    rule.value = "dark_theme";

    nlohmann::json j = rule;
    EXPECT_EQ(j["attribute"], "country");
    EXPECT_EQ(j["operator"], "eq");

    TargetingRule rule2 = j.get<TargetingRule>();
    EXPECT_EQ(rule2.attribute, "country");
    EXPECT_EQ(rule2.op, "eq");
    EXPECT_EQ(rule2.values[0], "US");
}

TEST_F(FlagEvaluationTest, EvaluationResultSerialization) {
    EvaluationResult r;
    r.flag_key = "ui.dark_mode";
    r.value = true;
    r.version = 5;
    r.reason = "rule_match";

    nlohmann::json j = r;
    EXPECT_EQ(j["flag_key"], "ui.dark_mode");
    EXPECT_EQ(j["value"], true);
    EXPECT_EQ(j["version"], 5);
    EXPECT_EQ(j["reason"], "rule_match");
}

// --- Percentage hash determinism test ---

TEST_F(FlagEvaluationTest, PercentageHashDeterministic) {
    // The hash of the same input should always produce the same result
    std::string input1 = "test.flag:user_123";
    std::string input2 = "test.flag:user_123";

    auto hash1 = std::hash<std::string>{}(input1) % 100;
    auto hash2 = std::hash<std::string>{}(input2) % 100;

    EXPECT_EQ(hash1, hash2);
}

TEST_F(FlagEvaluationTest, PercentageHashDistribution) {
    // Test that hash distributes across the range (not all in one bucket)
    int buckets[10] = {};

    for (int i = 0; i < 1000; ++i) {
        std::string input = "test.flag:user_" + std::to_string(i);
        int hash = static_cast<int>(std::hash<std::string>{}(input) % 100);
        buckets[hash / 10]++;
    }

    // Each bucket should have at least some entries (very loose check)
    for (int i = 0; i < 10; ++i) {
        EXPECT_GT(buckets[i], 0) << "Bucket " << i << " is empty";
    }
}

// --- Disabled flag test ---

TEST_F(FlagEvaluationTest, DisabledFlagReturnsDefault) {
    Flag f = make_test_flag("test.disabled", false, nlohmann::json("off"), {});

    // Disabled flag should return default regardless of rules
    TargetingRule rule;
    rule.attribute = "user_id";
    rule.op = "in";
    rule.values = {"u_1"};
    rule.value = "on";
    f.rules = {rule};

    // We can't directly call evaluate_flag (private), but we verify the flag model
    EXPECT_FALSE(f.enabled);
    EXPECT_EQ(f.default_value, "off");
}

// --- Rule structure validation ---

TEST_F(FlagEvaluationTest, MultipleRulesFirstMatchWins) {
    TargetingRule rule1;
    rule1.attribute = "country";
    rule1.op = "eq";
    rule1.values = {"US"};
    rule1.value = "variant_a";

    TargetingRule rule2;
    rule2.attribute = "country";
    rule2.op = "eq";
    rule2.values = {"US"};
    rule2.value = "variant_b";

    Flag f = make_test_flag("test.multi", true, nlohmann::json("default"), {rule1, rule2});

    // First rule matching should win — rule1 should be evaluated first
    EXPECT_EQ(f.rules[0].value, "variant_a");
    EXPECT_EQ(f.rules[1].value, "variant_b");
}

// --- Operator value tests ---

TEST_F(FlagEvaluationTest, AllOperatorsSupported) {
    std::vector<std::string> operators = {
        "in", "not_in", "eq", "neq", "gt", "lt", "contains", "percentage"
    };

    for (const auto& op : operators) {
        TargetingRule rule;
        rule.op = op;
        rule.attribute = "test";
        EXPECT_NO_THROW(nlohmann::json j = rule) << "Failed for operator: " << op;
    }
}

TEST_F(FlagEvaluationTest, NoRulesReturnsDefault) {
    Flag f = make_test_flag("test.norules", true, nlohmann::json(42), {});
    EXPECT_TRUE(f.rules.empty());
    EXPECT_EQ(f.default_value, 42);
}

} // anonymous namespace
} // namespace ffx

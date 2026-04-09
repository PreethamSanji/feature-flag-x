#include "repositories/flag_repo.h"
#include <format>

namespace ffx {

FlagRepo::FlagRepo(ConnectionPool& pool) : pool_(pool) {}

Flag FlagRepo::row_to_flag(const pqxx::row& row) {
    Flag f;
    f.id            = row["id"].as<std::string>();
    f.key           = row["key"].as<std::string>();
    f.description   = row["description"].as<std::string>();
    f.flag_type     = flag_type_from_string(row["flag_type"].as<std::string>());
    f.default_value = nlohmann::json::parse(row["default_value"].as<std::string>());
    f.rules         = nlohmann::json::parse(row["rules"].as<std::string>())
                          .get<std::vector<TargetingRule>>();
    f.enabled       = row["enabled"].as<bool>();
    f.version       = row["version"].as<int>();
    f.environment   = row["environment"].as<std::string>();
    f.created_at    = row["created_at"].as<std::string>();
    f.updated_at    = row["updated_at"].as<std::string>();
    return f;
}

std::expected<std::vector<Flag>, std::string> FlagRepo::list(const std::string& environment) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        pqxx::result result;
        if (environment.empty()) {
            result = txn.exec("SELECT * FROM flags ORDER BY key");
        } else {
            result = txn.exec_params(
                "SELECT * FROM flags WHERE environment = $1 ORDER BY key",
                environment);
        }

        std::vector<Flag> flags;
        flags.reserve(result.size());
        for (const auto& row : result) {
            flags.push_back(row_to_flag(row));
        }
        txn.commit();
        return flags;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to list flags: {}", e.what()));
    }
}

std::expected<std::optional<Flag>, std::string> FlagRepo::find_by_key(
    const std::string& key, const std::string& environment) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "SELECT * FROM flags WHERE key = $1 AND environment = $2",
            key, environment);

        txn.commit();

        if (result.empty()) {
            return std::optional<Flag>(std::nullopt);
        }
        return std::optional<Flag>(row_to_flag(result[0]));
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to find flag: {}", e.what()));
    }
}

std::expected<Flag, std::string> FlagRepo::create(const Flag& flag) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        nlohmann::json rules_json = flag.rules;
        auto result = txn.exec_params(
            "INSERT INTO flags (key, description, flag_type, default_value, rules, enabled, environment) "
            "VALUES ($1, $2, $3, $4::jsonb, $5::jsonb, $6, $7) RETURNING *",
            flag.key,
            flag.description,
            flag_type_to_string(flag.flag_type),
            flag.default_value.dump(),
            rules_json.dump(),
            flag.enabled,
            flag.environment);

        txn.commit();

        if (result.empty()) {
            return std::unexpected(std::string("Insert returned no rows"));
        }
        return row_to_flag(result[0]);
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to create flag: {}", e.what()));
    }
}

std::expected<Flag, std::string> FlagRepo::update(
    const std::string& key, const std::string& environment, const Flag& flag) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        nlohmann::json rules_json = flag.rules;
        auto result = txn.exec_params(
            "UPDATE flags SET description = $1, flag_type = $2, default_value = $3::jsonb, "
            "rules = $4::jsonb, enabled = $5, version = version + 1, updated_at = NOW() "
            "WHERE key = $6 AND environment = $7 RETURNING *",
            flag.description,
            flag_type_to_string(flag.flag_type),
            flag.default_value.dump(),
            rules_json.dump(),
            flag.enabled,
            key,
            environment);

        txn.commit();

        if (result.empty()) {
            return std::unexpected(std::string("Flag not found"));
        }
        return row_to_flag(result[0]);
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to update flag: {}", e.what()));
    }
}

std::expected<void, std::string> FlagRepo::remove(
    const std::string& key, const std::string& environment) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "DELETE FROM flags WHERE key = $1 AND environment = $2",
            key, environment);

        txn.commit();

        if (result.affected_rows() == 0) {
            return std::unexpected(std::string("Flag not found"));
        }
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to delete flag: {}", e.what()));
    }
}

std::expected<Flag, std::string> FlagRepo::toggle(
    const std::string& key, const std::string& environment) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        auto result = txn.exec_params(
            "UPDATE flags SET enabled = NOT enabled, version = version + 1, updated_at = NOW() "
            "WHERE key = $1 AND environment = $2 RETURNING *",
            key, environment);

        txn.commit();

        if (result.empty()) {
            return std::unexpected(std::string("Flag not found"));
        }
        return row_to_flag(result[0]);
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Failed to toggle flag: {}", e.what()));
    }
}

} // namespace ffx

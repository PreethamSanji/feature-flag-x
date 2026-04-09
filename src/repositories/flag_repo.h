#pragma once

#include "models/flag.h"
#include "repositories/connection_pool.h"
#include <expected>
#include <optional>
#include <vector>
#include <string>

namespace ffx {

/// Repository for feature flag CRUD operations.
class FlagRepo {
public:
    explicit FlagRepo(ConnectionPool& pool);

    /// List all flags, optionally filtered by environment.
    std::expected<std::vector<Flag>, std::string> list(const std::string& environment = "");

    /// Find a flag by its key and environment.
    std::expected<std::optional<Flag>, std::string> find_by_key(
        const std::string& key, const std::string& environment = "production");

    /// Create a new flag.
    std::expected<Flag, std::string> create(const Flag& flag);

    /// Update an existing flag (increments version).
    std::expected<Flag, std::string> update(const std::string& key, const std::string& environment, const Flag& flag);

    /// Delete a flag by key and environment.
    std::expected<void, std::string> remove(const std::string& key, const std::string& environment);

    /// Toggle a flag's enabled state.
    std::expected<Flag, std::string> toggle(const std::string& key, const std::string& environment);

private:
    Flag row_to_flag(const pqxx::row& row);
    ConnectionPool& pool_;
};

} // namespace ffx

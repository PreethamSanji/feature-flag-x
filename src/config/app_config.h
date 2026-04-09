#pragma once

#include <string>
#include <cstdint>

namespace ffx {

/// Application configuration loaded from environment variables.
struct AppConfig {
    std::string db_host = "localhost";
    uint16_t    db_port = 5432;
    std::string db_name = "featureflagx";
    std::string db_user = "ffx";
    std::string db_password = "ffx_secret";
    int         db_pool_size = 10;

    std::string redis_host = "localhost";
    uint16_t    redis_port = 6379;

    std::string jwt_secret = "change-me-in-production";
    int         jwt_expiry_seconds = 3600;

    uint16_t    server_port = 8080;
    int         server_threads = 4;

    /// Build a libpqxx connection string.
    std::string connection_string() const;
};

/// Load configuration from environment variables with defaults.
AppConfig load_config();

} // namespace ffx

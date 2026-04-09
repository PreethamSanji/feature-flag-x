#include "config/app_config.h"
#include <cstdlib>
#include <format>

namespace ffx {

namespace {

std::string env_or(const char* name, const std::string& fallback) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : fallback;
}

int env_int(const char* name, int fallback) {
    const char* val = std::getenv(name);
    return val ? std::stoi(val) : fallback;
}

} // anonymous namespace

std::string AppConfig::connection_string() const {
    return std::format(
        "host={} port={} dbname={} user={} password={}",
        db_host, db_port, db_name, db_user, db_password
    );
}

AppConfig load_config() {
    AppConfig cfg;
    cfg.db_host          = env_or("FFX_DB_HOST", cfg.db_host);
    cfg.db_port          = static_cast<uint16_t>(env_int("FFX_DB_PORT", cfg.db_port));
    cfg.db_name          = env_or("FFX_DB_NAME", cfg.db_name);
    cfg.db_user          = env_or("FFX_DB_USER", cfg.db_user);
    cfg.db_password      = env_or("FFX_DB_PASSWORD", cfg.db_password);
    cfg.db_pool_size     = env_int("FFX_DB_POOL_SIZE", cfg.db_pool_size);
    cfg.redis_host       = env_or("FFX_REDIS_HOST", cfg.redis_host);
    cfg.redis_port       = static_cast<uint16_t>(env_int("FFX_REDIS_PORT", cfg.redis_port));
    cfg.jwt_secret       = env_or("FFX_JWT_SECRET", cfg.jwt_secret);
    cfg.jwt_expiry_seconds = env_int("FFX_JWT_EXPIRY", cfg.jwt_expiry_seconds);
    cfg.server_port      = static_cast<uint16_t>(env_int("FFX_SERVER_PORT", cfg.server_port));
    cfg.server_threads   = env_int("FFX_SERVER_THREADS", cfg.server_threads);
    return cfg;
}

} // namespace ffx

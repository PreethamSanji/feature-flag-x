#include "services/service_registry.h"
#include <stdexcept>

namespace ffx {

std::unique_ptr<ServiceRegistry> ServiceRegistry::instance_;

void ServiceRegistry::init(const AppConfig& config) {
    instance_ = std::unique_ptr<ServiceRegistry>(new ServiceRegistry());
    instance_->config_ = config;

    instance_->pool_ = std::make_unique<ConnectionPool>(
        config.connection_string(), config.db_pool_size);

    instance_->flag_repo_ = std::make_unique<FlagRepo>(*instance_->pool_);
    instance_->audit_repo_ = std::make_unique<AuditRepo>(*instance_->pool_);

    instance_->flag_service_ = std::make_unique<FlagService>(
        *instance_->flag_repo_, *instance_->audit_repo_);

    instance_->cache_service_ = std::make_unique<CacheService>(
        config.redis_host, config.redis_port);

    instance_->auth_service_ = std::make_unique<AuthService>(
        *instance_->pool_, config.jwt_secret, config.jwt_expiry_seconds);
}

ServiceRegistry& ServiceRegistry::instance() {
    if (!instance_) {
        throw std::runtime_error("ServiceRegistry not initialized. Call init() first.");
    }
    return *instance_;
}

} // namespace ffx

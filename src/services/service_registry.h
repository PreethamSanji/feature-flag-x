#pragma once

#include "config/app_config.h"
#include "repositories/connection_pool.h"
#include "repositories/flag_repo.h"
#include "repositories/audit_repo.h"
#include "services/flag_service.h"
#include "services/cache_service.h"
#include "services/auth_service.h"
#include <memory>

namespace ffx {

// Forward declaration
class PropagationService;

/// Singleton registry providing access to all application services.
class ServiceRegistry {
public:
    /// Initialize the singleton with the given config.
    static void init(const AppConfig& config);

    /// Get the singleton instance (must call init first).
    static ServiceRegistry& instance();

    FlagService& flag_service() { return *flag_service_; }
    CacheService& cache_service() { return *cache_service_; }
    AuthService& auth_service() { return *auth_service_; }
    FlagRepo& flag_repo() { return *flag_repo_; }
    AuditRepo& audit_repo() { return *audit_repo_; }
    ConnectionPool& connection_pool() { return *pool_; }
    const AppConfig& config() const { return config_; }

private:
    ServiceRegistry() = default;

    AppConfig config_;
    std::unique_ptr<ConnectionPool> pool_;
    std::unique_ptr<FlagRepo> flag_repo_;
    std::unique_ptr<AuditRepo> audit_repo_;
    std::unique_ptr<FlagService> flag_service_;
    std::unique_ptr<CacheService> cache_service_;
    std::unique_ptr<AuthService> auth_service_;

    static std::unique_ptr<ServiceRegistry> instance_;
};

} // namespace ffx

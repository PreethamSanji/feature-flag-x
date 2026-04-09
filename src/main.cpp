#include "config/app_config.h"
#include "services/service_registry.h"
#include <drogon/drogon.h>
#include <iostream>

int main() {
    std::cout << "FeatureFlagX starting...\n";

    // Load configuration from environment
    auto config = ffx::load_config();
    std::cout << "Config loaded: db=" << config.db_host << ":" << config.db_port
              << " redis=" << config.redis_host << ":" << config.redis_port << "\n";

    // Initialize all services
    try {
        ffx::ServiceRegistry::init(config);
        std::cout << "Services initialized successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize services: " << e.what() << "\n";
        return 1;
    }

    // Check Redis health
    if (ffx::ServiceRegistry::instance().cache_service().is_healthy()) {
        std::cout << "Redis connection: healthy\n";
    } else {
        std::cerr << "Warning: Redis connection unavailable, running without cache\n";
    }

    // Configure and start Drogon
    drogon::app()
        .setLogPath("./")
        .setLogLevel(trantor::Logger::kInfo)
        .addListener("0.0.0.0", config.server_port)
        .setThreadNum(config.server_threads)
        .enableRunAsDaemon(false);

    std::cout << "Starting server on port " << config.server_port
              << " with " << config.server_threads << " threads\n";

    drogon::app().run();
    return 0;
}

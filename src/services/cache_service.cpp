#include "services/cache_service.h"
#include <sw/redis++/redis++.h>
#include <chrono>
#include <iostream>
#include <format>

namespace ffx {

CacheService::CacheService(const std::string& host, uint16_t port) {
    try {
        sw::redis::ConnectionOptions opts;
        opts.host = host;
        opts.port = static_cast<int>(port);
        opts.socket_timeout = std::chrono::milliseconds(500);
        opts.connect_timeout = std::chrono::milliseconds(1000);
        redis_ = std::make_unique<sw::redis::Redis>(opts);
    } catch (const std::exception& e) {
        std::cerr << "Redis connection failed: " << e.what() << "\n";
        redis_.reset();
    }
}

CacheService::~CacheService() = default;

std::string CacheService::cache_key(const std::string& environment, const std::string& key) {
    return std::format("ffx:flag:{}:{}", environment, key);
}

std::optional<Flag> CacheService::get_flag(const std::string& environment, const std::string& key) {
    if (!redis_) return std::nullopt;

    try {
        auto val = redis_->get(cache_key(environment, key));
        if (!val) return std::nullopt;

        auto j = nlohmann::json::parse(*val);
        return j.get<Flag>();
    } catch (const std::exception& e) {
        std::cerr << "Cache get error: " << e.what() << "\n";
        return std::nullopt;
    }
}

void CacheService::set_flag(const Flag& flag) {
    if (!redis_) return;

    try {
        nlohmann::json j = flag;
        redis_->setex(
            cache_key(flag.environment, flag.key),
            ttl_seconds_,
            j.dump());
    } catch (const std::exception& e) {
        std::cerr << "Cache set error: " << e.what() << "\n";
    }
}

void CacheService::invalidate(const std::string& environment, const std::string& key) {
    if (!redis_) return;

    try {
        redis_->del(cache_key(environment, key));
    } catch (const std::exception& e) {
        std::cerr << "Cache invalidate error: " << e.what() << "\n";
    }
}

bool CacheService::is_healthy() const {
    if (!redis_) return false;

    try {
        auto reply = redis_->ping();
        return reply == "PONG";
    } catch (...) {
        return false;
    }
}

} // namespace ffx

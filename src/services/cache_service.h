#pragma once

#include "models/flag.h"
#include <optional>
#include <string>
#include <memory>

namespace sw::redis { class Redis; }

namespace ffx {

/// Redis-backed cache service with read-through and graceful degradation.
class CacheService {
public:
    CacheService(const std::string& host, uint16_t port);
    ~CacheService();

    /// Get a cached flag. Returns nullopt on miss or Redis error.
    std::optional<Flag> get_flag(const std::string& environment, const std::string& key);

    /// Cache a flag with TTL.
    void set_flag(const Flag& flag);

    /// Invalidate a cached flag.
    void invalidate(const std::string& environment, const std::string& key);

    /// Check if the Redis connection is healthy.
    bool is_healthy() const;

private:
    static std::string cache_key(const std::string& environment, const std::string& key);

    std::unique_ptr<sw::redis::Redis> redis_;
    static constexpr int ttl_seconds_ = 60;
};

} // namespace ffx

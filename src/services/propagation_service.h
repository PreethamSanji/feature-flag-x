#pragma once

#include "services/cache_service.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace sw::redis { class Redis; class Subscriber; }

namespace ffx {

/// Redis Pub/Sub service for real-time flag change propagation.
class PropagationService {
public:
    PropagationService(const std::string& host, uint16_t port);
    ~PropagationService();

    PropagationService(const PropagationService&) = delete;
    PropagationService& operator=(const PropagationService&) = delete;

    /// Publish a flag change event.
    void publish_change(const std::string& key, const std::string& environment, int version);

    /// Start the background subscriber thread.
    void start_subscriber(CacheService& cache);

    /// Stop the subscriber and join the thread.
    void stop();

private:
    std::unique_ptr<sw::redis::Redis> publisher_;
    std::unique_ptr<sw::redis::Redis> subscriber_redis_;
    std::thread subscriber_thread_;
    std::atomic<bool> running_{false};

    static constexpr const char* channel_ = "ffx:flag_updates";
};

} // namespace ffx

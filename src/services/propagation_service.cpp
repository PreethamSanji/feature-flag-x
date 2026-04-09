#include "services/propagation_service.h"
#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <format>
#include <chrono>

namespace ffx {

PropagationService::PropagationService(const std::string& host, uint16_t port) {
    sw::redis::ConnectionOptions opts;
    opts.host = host;
    opts.port = static_cast<int>(port);
    opts.socket_timeout = std::chrono::milliseconds(500);
    opts.connect_timeout = std::chrono::milliseconds(1000);

    try {
        publisher_ = std::make_unique<sw::redis::Redis>(opts);
        // Subscriber needs its own connection
        subscriber_redis_ = std::make_unique<sw::redis::Redis>(opts);
    } catch (const std::exception& e) {
        std::cerr << "Propagation service Redis connection failed: " << e.what() << "\n";
    }
}

PropagationService::~PropagationService() {
    stop();
}

void PropagationService::publish_change(const std::string& key, const std::string& environment, int version) {
    if (!publisher_) return;

    try {
        nlohmann::json msg = {
            {"event", "flag_updated"},
            {"key", key},
            {"env", environment},
            {"version", version}
        };
        publisher_->publish(channel_, msg.dump());
    } catch (const std::exception& e) {
        std::cerr << "Failed to publish flag change: " << e.what() << "\n";
    }
}

void PropagationService::start_subscriber(CacheService& cache) {
    if (!subscriber_redis_) {
        std::cerr << "Cannot start subscriber: no Redis connection\n";
        return;
    }

    running_ = true;

    subscriber_thread_ = std::thread([this, &cache] {
        try {
            auto sub = subscriber_redis_->subscriber();

            sub.on_message([&cache](std::string channel, std::string msg) {
                try {
                    auto j = nlohmann::json::parse(msg);
                    auto key = j.at("key").get<std::string>();
                    auto env = j.at("env").get<std::string>();
                    cache.invalidate(env, key);
                    std::cout << std::format("Cache invalidated: {}:{}\n", env, key);
                } catch (const std::exception& e) {
                    std::cerr << "Error processing propagation message: " << e.what() << "\n";
                }
            });

            sub.subscribe(channel_);

            while (running_) {
                try {
                    sub.consume();
                } catch (const sw::redis::TimeoutError&) {
                    // Expected on timeout, just loop
                } catch (const std::exception& e) {
                    if (running_) {
                        std::cerr << "Subscriber error: " << e.what() << "\n";
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            }

            sub.unsubscribe(channel_);
        } catch (const std::exception& e) {
            std::cerr << "Subscriber thread failed: " << e.what() << "\n";
        }
    });

    std::cout << "Propagation subscriber started on channel: " << channel_ << "\n";
}

void PropagationService::stop() {
    running_ = false;
    if (subscriber_thread_.joinable()) {
        subscriber_thread_.join();
    }
}

} // namespace ffx

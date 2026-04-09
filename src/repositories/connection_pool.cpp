#include "repositories/connection_pool.h"
#include <stdexcept>

namespace ffx {

ConnectionPool::ConnectionPool(const std::string& conn_str, int pool_size) {
    available_.reserve(pool_size);
    for (int i = 0; i < pool_size; ++i) {
        available_.push_back(std::make_shared<pqxx::connection>(conn_str));
    }
}

ConnectionPool::~ConnectionPool() = default;

ConnectionPool::ConnectionGuard ConnectionPool::acquire() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return !available_.empty(); });

    auto conn = available_.back();
    available_.pop_back();
    return ConnectionGuard(std::move(conn), *this);
}

void ConnectionPool::release(std::shared_ptr<pqxx::connection> conn) {
    {
        std::lock_guard lock(mutex_);
        available_.push_back(std::move(conn));
    }
    cv_.notify_one();
}

// ConnectionGuard

ConnectionPool::ConnectionGuard::ConnectionGuard(
    std::shared_ptr<pqxx::connection> conn, ConnectionPool& pool)
    : conn_(std::move(conn)), pool_(&pool) {}

ConnectionPool::ConnectionGuard::~ConnectionGuard() {
    if (conn_ && pool_) {
        pool_->release(std::move(conn_));
    }
}

ConnectionPool::ConnectionGuard::ConnectionGuard(ConnectionGuard&& other) noexcept
    : conn_(std::move(other.conn_)), pool_(other.pool_) {
    other.pool_ = nullptr;
}

} // namespace ffx

#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <pqxx/pqxx>

namespace ffx {

/// RAII connection pool for thread-safe PostgreSQL access.
class ConnectionPool {
public:
    /// Create a pool with the given connection string and size.
    ConnectionPool(const std::string& conn_str, int pool_size);
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    /// RAII guard that returns the connection to the pool on destruction.
    class ConnectionGuard {
    public:
        ConnectionGuard(std::shared_ptr<pqxx::connection> conn, ConnectionPool& pool);
        ~ConnectionGuard();

        ConnectionGuard(ConnectionGuard&& other) noexcept;
        ConnectionGuard& operator=(ConnectionGuard&&) = delete;
        ConnectionGuard(const ConnectionGuard&) = delete;
        ConnectionGuard& operator=(const ConnectionGuard&) = delete;

        pqxx::connection& operator*() { return *conn_; }
        pqxx::connection* operator->() { return conn_.get(); }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        ConnectionPool* pool_;
    };

    /// Acquire a connection from the pool (blocks if none available).
    ConnectionGuard acquire();

private:
    void release(std::shared_ptr<pqxx::connection> conn);

    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::shared_ptr<pqxx::connection>> available_;
};

} // namespace ffx

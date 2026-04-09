#include <gtest/gtest.h>
#include "services/cache_service.h"
#include "models/flag.h"

namespace ffx {
namespace {

class CacheServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Try to connect to Redis — skip tests if unavailable
        cache_ = std::make_unique<CacheService>("localhost", 6379);
        if (!cache_->is_healthy()) {
            GTEST_SKIP() << "Redis not available, skipping cache tests";
        }
    }

    std::unique_ptr<CacheService> cache_;
};

TEST_F(CacheServiceTest, SetAndGetRoundtrip) {
    Flag f;
    f.id = "cache-test-id";
    f.key = "cache.test.flag";
    f.description = "A test flag for cache";
    f.flag_type = FlagType::boolean_type;
    f.default_value = true;
    f.enabled = true;
    f.version = 1;
    f.environment = "test";
    f.created_at = "2026-01-01T00:00:00Z";
    f.updated_at = "2026-01-01T00:00:00Z";

    cache_->set_flag(f);

    auto result = cache_->get_flag("test", "cache.test.flag");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->key, "cache.test.flag");
    EXPECT_EQ(result->version, 1);
    EXPECT_EQ(result->default_value, true);

    // Cleanup
    cache_->invalidate("test", "cache.test.flag");
}

TEST_F(CacheServiceTest, MissReturnsNullopt) {
    auto result = cache_->get_flag("test", "nonexistent.flag.key");
    EXPECT_FALSE(result.has_value());
}

TEST_F(CacheServiceTest, InvalidateRemovesKey) {
    Flag f;
    f.id = "cache-inv-id";
    f.key = "cache.invalidate.test";
    f.flag_type = FlagType::boolean_type;
    f.default_value = false;
    f.enabled = true;
    f.version = 1;
    f.environment = "test";
    f.created_at = "2026-01-01T00:00:00Z";
    f.updated_at = "2026-01-01T00:00:00Z";

    cache_->set_flag(f);

    // Verify it exists
    auto before = cache_->get_flag("test", "cache.invalidate.test");
    ASSERT_TRUE(before.has_value());

    // Invalidate
    cache_->invalidate("test", "cache.invalidate.test");

    // Verify it's gone
    auto after = cache_->get_flag("test", "cache.invalidate.test");
    EXPECT_FALSE(after.has_value());
}

TEST_F(CacheServiceTest, CacheKeyFormat) {
    // Verify different environments produce different cache entries
    Flag f1;
    f1.id = "env-test-1";
    f1.key = "env.test.flag";
    f1.flag_type = FlagType::boolean_type;
    f1.default_value = true;
    f1.enabled = true;
    f1.version = 1;
    f1.environment = "production";
    f1.created_at = "2026-01-01T00:00:00Z";
    f1.updated_at = "2026-01-01T00:00:00Z";

    Flag f2 = f1;
    f2.id = "env-test-2";
    f2.environment = "staging";
    f2.default_value = false;
    f2.version = 2;

    cache_->set_flag(f1);
    cache_->set_flag(f2);

    auto prod = cache_->get_flag("production", "env.test.flag");
    auto staging = cache_->get_flag("staging", "env.test.flag");

    ASSERT_TRUE(prod.has_value());
    ASSERT_TRUE(staging.has_value());
    EXPECT_EQ(prod->default_value, true);
    EXPECT_EQ(staging->default_value, false);

    // Cleanup
    cache_->invalidate("production", "env.test.flag");
    cache_->invalidate("staging", "env.test.flag");
}

// Test graceful degradation when Redis is unavailable
TEST(CacheServiceGracefulTest, UnavailableRedisReturnsNullopt) {
    CacheService bad_cache("invalid-host-that-does-not-exist", 1);
    auto result = bad_cache.get_flag("test", "any.key");
    EXPECT_FALSE(result.has_value());
}

TEST(CacheServiceGracefulTest, UnavailableRedisSetDoesNotThrow) {
    CacheService bad_cache("invalid-host-that-does-not-exist", 1);
    Flag f;
    f.key = "test";
    f.environment = "test";
    EXPECT_NO_THROW(bad_cache.set_flag(f));
}

TEST(CacheServiceGracefulTest, UnavailableRedisInvalidateDoesNotThrow) {
    CacheService bad_cache("invalid-host-that-does-not-exist", 1);
    EXPECT_NO_THROW(bad_cache.invalidate("test", "key"));
}

} // anonymous namespace
} // namespace ffx

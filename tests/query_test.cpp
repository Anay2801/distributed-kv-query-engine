#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "kv_store.hpp"
#include "kv_filter.hpp"
#include "wal_store.hpp"

class QueryTest : public ::testing::Test {
protected:
    KVStore store;
};

TEST_F(QueryTest, ScanKeyPrefix) {
    store.set("app:one", "1");
    store.set("app:two", "2");
    store.set("other", "3");

    auto results = store.scan(filter::key_prefix("app:"));
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, "app:one");
    EXPECT_EQ(results[1].first, "app:two");
}

TEST_F(QueryTest, ScanKeyContains) {
    store.set("hello_world", "1");
    store.set("hello_there", "2");
    store.set("bye", "3");

    auto results = store.scan(filter::key_contains("hello"));
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, "hello_there");
    EXPECT_EQ(results[1].first, "hello_world");
}

TEST_F(QueryTest, ScanValueEquals) {
    store.set("k1", "active");
    store.set("k2", "active");
    store.set("k3", "inactive");

    auto results = store.scan(filter::value_equals("active"));
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, "k1");
    EXPECT_EQ(results[1].first, "k2");
}

TEST_F(QueryTest, ScanValuePrefix) {
    store.set("k1", "user:alice");
    store.set("k2", "user:bob");
    store.set("k3", "admin:charlie");

    auto results = store.scan(filter::value_prefix("user:"));
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, "k1");
    EXPECT_EQ(results[1].first, "k2");
}

TEST_F(QueryTest, ScanReturnsEmptyWhenNoMatch) {
    store.set("key1", "value1");
    auto results = store.scan(filter::key_prefix("nonexistent:"));
    EXPECT_TRUE(results.empty());
}

TEST_F(QueryTest, ScanAllEntries) {
    store.set("a", "1");
    store.set("b", "2");
    store.set("c", "3");

    auto results = store.scan([](const std::string&, const std::string&) { return true; });
    EXPECT_EQ(results.size(), 3u);
}

TEST_F(QueryTest, ScanAfterDelete) {
    store.set("key1", "val1");
    store.set("key2", "val2");
    store.del("key1");

    auto results = store.scan(filter::key_prefix("key"));
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].first, "key2");
}

TEST_F(QueryTest, ScanThroughWALStore) {
    const std::string log_path = "/tmp/kv_query_wal_test.log";
    std::remove(log_path.c_str());

    WALStore wal(log_path);
    wal.set("user:alice", "active");
    wal.set("user:bob", "inactive");
    wal.set("admin:charlie", "active");

    auto results = wal.scan(filter::value_equals("active"));
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, "admin:charlie");
    EXPECT_EQ(results[1].first, "user:alice");

    std::remove(log_path.c_str());
}

TEST_F(QueryTest, CombinedLambdaFilter) {
    store.set("x:1", "yes");
    store.set("x:2", "no");
    store.set("y:3", "yes");

    auto results = store.scan([](const std::string& k, const std::string& v) {
        return !k.empty() && k[0] == 'x' && v == "yes";
    });

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].first, "x:1");
    EXPECT_EQ(results[0].second, "yes");
}

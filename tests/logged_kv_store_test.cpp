#include <gtest/gtest.h>
#include <fstream>
#include "logged_kv_store.hpp"

class LoggedKVStoreTest : public ::testing::Test {
protected:
    const std::string log_path = "/tmp/kv_persist_test.log";

    void SetUp() override { std::remove(log_path.c_str()); }
    void TearDown() override { std::remove(log_path.c_str()); }
};

TEST_F(LoggedKVStoreTest, SetAndGetWork) {
    LoggedKVStore store(log_path);
    store.set("k", "v");
    EXPECT_EQ(store.get("k"), "v");
}

TEST_F(LoggedKVStoreTest, LogFileContainsSetEntry) {
    LoggedKVStore store(log_path);
    store.set("k", "v");
    std::ifstream f(log_path);
    std::string line;
    std::getline(f, line);
    EXPECT_EQ(line, "SET k v");
}

TEST_F(LoggedKVStoreTest, RebuildRestoresData) {
    {
        LoggedKVStore store(log_path);
        store.set("key1", "value1");
        store.set("key2", "value2");
    }
    LoggedKVStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), "value1");
    EXPECT_EQ(store2.get("key2"), "value2");
}

TEST_F(LoggedKVStoreTest, RebuildRespectsDeletes) {
    {
        LoggedKVStore store(log_path);
        store.set("key1", "value1");
        store.del("key1");
    }
    LoggedKVStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), std::nullopt);
}

TEST_F(LoggedKVStoreTest, RebuildUsesLatestValueForKey) {
    {
        LoggedKVStore store(log_path);
        store.set("key1", "v1");
        store.set("key1", "v2");
    }
    LoggedKVStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), "v2");
}

TEST_F(LoggedKVStoreTest, FreshStartWithNoLogFile) {
    LoggedKVStore store(log_path);
    EXPECT_EQ(store.get("missing"), std::nullopt);
}

TEST_F(LoggedKVStoreTest, DelMissingKeyDoesNotWriteToLog) {
    LoggedKVStore store(log_path);
    EXPECT_FALSE(store.del("missing"));
    std::ifstream f(log_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty());
}

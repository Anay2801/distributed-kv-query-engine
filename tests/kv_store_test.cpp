#include <gtest/gtest.h>
#include "kv_store.hpp"

TEST(KVStoreTest, SetThenGetReturnsValue) {
    KVStore store;
    store.set("key1", "value1");
    EXPECT_EQ(store.get("key1"), "value1");
}

TEST(KVStoreTest, GetMissingKeyReturnsNullopt) {
    KVStore store;
    EXPECT_EQ(store.get("missing"), std::nullopt);
}

TEST(KVStoreTest, SetOverwritesExistingKey) {
    KVStore store;
    store.set("key1", "value1");
    store.set("key1", "value2");
    EXPECT_EQ(store.get("key1"), "value2");
}

TEST(KVStoreTest, DelExistingKeyReturnsTrueAndRemovesIt) {
    KVStore store;
    store.set("key1", "value1");
    EXPECT_TRUE(store.del("key1"));
    EXPECT_EQ(store.get("key1"), std::nullopt);
}

TEST(KVStoreTest, DelMissingKeyReturnsFalse) {
    KVStore store;
    EXPECT_FALSE(store.del("missing"));
}

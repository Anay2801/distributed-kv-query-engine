#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <vector>
#include "wal_store.hpp"
#include "wal_checksum.hpp"

class WALStoreTest : public ::testing::Test {
protected:
    const std::string log_path = "/tmp/kv_wal_test.log";

    void SetUp() override { std::remove(log_path.c_str()); }
    void TearDown() override { std::remove(log_path.c_str()); }
};

TEST_F(WALStoreTest, SetAndGetWork) {
    WALStore store(log_path);
    store.set("k", "v");
    EXPECT_EQ(store.get("k"), "v");
}

TEST_F(WALStoreTest, LogFileHasExpectedFormat) {
    WALStore store(log_path);
    store.set("k", "v");

    std::ifstream f(log_path);
    std::string line;
    std::getline(f, line);

    auto last_space = line.rfind(' ');
    ASSERT_NE(last_space, std::string::npos);
    std::string content = line.substr(0, last_space);
    std::string checksum_str = line.substr(last_space + 1);

    EXPECT_EQ(content, "0 SET k v");
    EXPECT_NO_THROW(std::stoul(checksum_str));
}

TEST_F(WALStoreTest, RebuildRestoresData) {
    {
        WALStore store(log_path);
        store.set("key1", "value1");
        store.set("key2", "value2");
    }
    WALStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), "value1");
    EXPECT_EQ(store2.get("key2"), "value2");
}

TEST_F(WALStoreTest, RebuildRespectsDeletes) {
    {
        WALStore store(log_path);
        store.set("key1", "value1");
        store.del("key1");
    }
    WALStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), std::nullopt);
}

TEST_F(WALStoreTest, RebuildUsesLatestValue) {
    {
        WALStore store(log_path);
        store.set("key1", "v1");
        store.set("key1", "v2");
    }
    WALStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), "v2");
}

TEST_F(WALStoreTest, CorruptedEntrySkippedDuringReplay) {
    {
        WALStore store(log_path);
        store.set("good", "data");
    }
    {
        std::ofstream f(log_path, std::ios::app);
        f << "1 SET bad_key bad_value NOTANUMBER\n";
    }
    WALStore store2(log_path);
    EXPECT_EQ(store2.get("good"), "data");
    EXPECT_EQ(store2.get("bad_key"), std::nullopt);
}

TEST_F(WALStoreTest, LSNIncrementsWithEachWrite) {
    WALStore store(log_path);
    store.set("a", "1");
    store.set("b", "2");
    store.del("a");

    std::ifstream f(log_path);
    std::string line;
    std::vector<uint64_t> lsns;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        uint64_t lsn;
        ss >> lsn;
        lsns.push_back(lsn);
    }
    ASSERT_EQ(lsns.size(), 3u);
    EXPECT_EQ(lsns[0], 0u);
    EXPECT_EQ(lsns[1], 1u);
    EXPECT_EQ(lsns[2], 2u);
}

TEST_F(WALStoreTest, DelMissingKeyReturnsFalseAndNoLogEntry) {
    WALStore store(log_path);
    EXPECT_FALSE(store.del("missing"));

    std::ifstream f(log_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty());
}

TEST_F(WALStoreTest, WALEntryReplayedAfterSimulatedCrash) {
    {
        WALStore store(log_path);
        store.set("key1", "val1");
    }
    // Simulate: WAL write + fsync completed but process crashed before memory apply
    {
        std::string content = "1 SET key2 val2";
        std::ofstream f(log_path, std::ios::app);
        f << content << " " << wal_checksum(content) << "\n";
    }
    WALStore store2(log_path);
    EXPECT_EQ(store2.get("key1"), "val1");
    EXPECT_EQ(store2.get("key2"), "val2");
}

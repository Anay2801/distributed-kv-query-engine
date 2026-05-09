#include <gtest/gtest.h>
#include <cstdio>
#include "wal_store.hpp"
#include "cli_dispatch.hpp"

class CliTest : public ::testing::Test {
protected:
    const std::string db_path = "/tmp/kv_cli_test.wal";
    WALStore* store{nullptr};

    void SetUp() override {
        std::remove(db_path.c_str());
        store = new WALStore(db_path);
    }
    void TearDown() override {
        delete store;
        std::remove(db_path.c_str());
    }
};

TEST_F(CliTest, SetAndGetRoundtrip) {
    EXPECT_EQ(run_command(*store, {"set", "hello", "world"}), "OK\n");
    EXPECT_EQ(run_command(*store, {"get", "hello"}), "world\n");
}

TEST_F(CliTest, GetMissingKeyReturnsNil) {
    EXPECT_EQ(run_command(*store, {"get", "missing"}), "(nil)\n");
}

TEST_F(CliTest, DelExistingKeyReturnsOK) {
    run_command(*store, {"set", "k", "v"});
    EXPECT_EQ(run_command(*store, {"del", "k"}), "OK\n");
    EXPECT_EQ(run_command(*store, {"get", "k"}), "(nil)\n");
}

TEST_F(CliTest, DelMissingKeyReturnsNil) {
    EXPECT_EQ(run_command(*store, {"del", "absent"}), "(nil)\n");
}

TEST_F(CliTest, ScanReturnsMatchingKeysSorted) {
    run_command(*store, {"set", "user:1", "alice"});
    run_command(*store, {"set", "user:2", "bob"});
    run_command(*store, {"set", "item:1", "widget"});
    std::string out = run_command(*store, {"scan", "user:"});
    EXPECT_EQ(out, "user:1 alice\nuser:2 bob\n");
}

TEST_F(CliTest, ScanNoMatchesReturnsEmpty) {
    EXPECT_EQ(run_command(*store, {"scan", "x:"}), "(empty)\n");
}

TEST_F(CliTest, SetMultiWordValue) {
    EXPECT_EQ(run_command(*store, {"set", "msg", "hello", "world"}), "OK\n");
    EXPECT_EQ(run_command(*store, {"get", "msg"}), "hello world\n");
}

TEST_F(CliTest, UnknownCommandReturnsError) {
    std::string out = run_command(*store, {"flurp"});
    EXPECT_EQ(out, "error: unknown command: flurp\n");
}

TEST_F(CliTest, SetMissingValueArgReturnsError) {
    std::string out = run_command(*store, {"set", "key"});
    EXPECT_EQ(out, "error: set requires <key> <value>\n");
}

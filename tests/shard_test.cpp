#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "consistent_hash_ring.hpp"
#include "node_server.hpp"
#include "node_client.hpp"
#include "shard_router.hpp"

// ── ConsistentHashRing tests ────────────────────────────────────────────────

TEST(ConsistentHashRingTest, EmptyRingReturnsEmptyString) {
    ConsistentHashRing ring;
    EXPECT_EQ(ring.get_node("any_key"), "");
}

TEST(ConsistentHashRingTest, SingleNodeAlwaysSelected) {
    ConsistentHashRing ring;
    ring.add_node("node0");
    EXPECT_EQ(ring.get_node("key1"), "node0");
    EXPECT_EQ(ring.get_node("key2"), "node0");
    EXPECT_EQ(ring.get_node("key3"), "node0");
}

TEST(ConsistentHashRingTest, SameKeyAlwaysGoesToSameNode) {
    ConsistentHashRing ring;
    ring.add_node("node0");
    ring.add_node("node1");
    std::string first = ring.get_node("consistent_key");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(ring.get_node("consistent_key"), first);
    }
}

TEST(ConsistentHashRingTest, DifferentKeysDistributeAcrossNodes) {
    ConsistentHashRing ring;
    ring.add_node("node0");
    ring.add_node("node1");
    std::set<std::string> seen;
    for (int i = 0; i < 200; ++i) {
        seen.insert(ring.get_node("key_" + std::to_string(i)));
    }
    EXPECT_EQ(seen.size(), 2u);
}

TEST(ConsistentHashRingTest, RemoveNodeReroutesKeys) {
    ConsistentHashRing ring;
    ring.add_node("node0");
    ring.add_node("node1");
    std::string test_key;
    for (int i = 0; i < 1000 && test_key.empty(); ++i) {
        std::string k = "k" + std::to_string(i);
        if (ring.get_node(k) == "node1") test_key = k;
    }
    ASSERT_FALSE(test_key.empty());
    ring.remove_node("node1");
    EXPECT_EQ(ring.get_node(test_key), "node0");
}

// ── NodeServer + NodeClient tests ───────────────────────────────────────────

class NodeServerTest : public ::testing::Test {
protected:
    const std::string log_path = "/tmp/kv_node_server_test.log";
    std::unique_ptr<NodeServer> server;
    std::unique_ptr<NodeClient> client;

    void SetUp() override {
        std::remove(log_path.c_str());
        server = std::make_unique<NodeServer>(0, log_path);
        server->start();
        client = std::make_unique<NodeClient>("127.0.0.1", server->port());
    }

    void TearDown() override {
        client.reset();
        server->stop();
        std::remove(log_path.c_str());
    }
};

TEST_F(NodeServerTest, SetAndGetViaTCP) {
    client->set("hello", "world");
    EXPECT_EQ(client->get("hello"), "world");
}

TEST_F(NodeServerTest, GetMissingKeyReturnsNil) {
    EXPECT_EQ(client->get("missing"), std::nullopt);
}

TEST_F(NodeServerTest, DelViaTCP) {
    client->set("k", "v");
    EXPECT_TRUE(client->del("k"));
    EXPECT_EQ(client->get("k"), std::nullopt);
}

TEST_F(NodeServerTest, ScanPrefixViaTCP) {
    client->set("app:one", "1");
    client->set("app:two", "2");
    client->set("other", "3");

    auto results = client->scan_prefix("app:");
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, "app:one");
    EXPECT_EQ(results[1].first, "app:two");
}

// ── ShardRouter tests ───────────────────────────────────────────────────────

class ShardRouterTest : public ::testing::Test {
protected:
    std::vector<std::string> log_paths;
    std::vector<std::unique_ptr<NodeServer>> servers;
    std::unique_ptr<ShardRouter> router;

    void SetUp() override {
        for (int i = 0; i < 3; ++i) {
            std::string lp = "/tmp/kv_shard_" + std::to_string(i) + "_test.log";
            std::remove(lp.c_str());
            log_paths.push_back(lp);
            servers.push_back(std::make_unique<NodeServer>(0, lp));
            servers.back()->start();
        }
        std::map<std::string, std::pair<std::string, int>> endpoints;
        for (int i = 0; i < 3; ++i) {
            endpoints["node" + std::to_string(i)] = {"127.0.0.1", servers[i]->port()};
        }
        router = std::make_unique<ShardRouter>(endpoints);
    }

    void TearDown() override {
        router.reset();
        for (auto& s : servers) s->stop();
        for (const auto& lp : log_paths) std::remove(lp.c_str());
        servers.clear();
        log_paths.clear();
    }
};

TEST_F(ShardRouterTest, RouterSetAndGet) {
    router->set("key1", "value1");
    EXPECT_EQ(router->get("key1"), "value1");
}

TEST_F(ShardRouterTest, KeysRouteConsistently) {
    router->set("foo", "bar");
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(router->get("foo"), "bar");
    }
}

TEST_F(ShardRouterTest, ScanAcrossShards) {
    router->set("app:alpha", "1");
    router->set("app:beta", "2");
    router->set("app:gamma", "3");
    router->set("other", "4");

    auto results = router->scan_prefix("app:");
    std::sort(results.begin(), results.end());

    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].first, "app:alpha");
    EXPECT_EQ(results[1].first, "app:beta");
    EXPECT_EQ(results[2].first, "app:gamma");
}

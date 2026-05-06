#pragma once
#include "consistent_hash_ring.hpp"
#include "node_client.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class ShardRouter {
public:
    explicit ShardRouter(const std::map<std::string, std::pair<std::string, int>>& node_endpoints,
                          int vnodes_per_node = 150);
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    std::vector<std::pair<std::string, std::string>> scan_prefix(const std::string& prefix);

private:
    NodeClient& client_for(const std::string& key);

    ConsistentHashRing ring_;
    std::map<std::string, std::unique_ptr<NodeClient>> clients_;
};

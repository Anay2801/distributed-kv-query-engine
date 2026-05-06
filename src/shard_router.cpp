#include "shard_router.hpp"

ShardRouter::ShardRouter(const std::map<std::string, std::pair<std::string, int>>& node_endpoints,
                          int vnodes_per_node)
    : ring_(vnodes_per_node) {
    for (const auto& [node_id, ep] : node_endpoints) {
        ring_.add_node(node_id);
        clients_[node_id] = std::make_unique<NodeClient>(ep.first, ep.second);
    }
}

NodeClient& ShardRouter::client_for(const std::string& key) {
    return *clients_.at(ring_.get_node(key));
}

void ShardRouter::set(const std::string& key, const std::string& value) {
    client_for(key).set(key, value);
}

std::optional<std::string> ShardRouter::get(const std::string& key) {
    return client_for(key).get(key);
}

bool ShardRouter::del(const std::string& key) {
    return client_for(key).del(key);
}

std::vector<std::pair<std::string, std::string>> ShardRouter::scan_prefix(const std::string& prefix) {
    std::vector<std::pair<std::string, std::string>> all;
    for (auto& [node_id, client] : clients_) {
        auto results = client->scan_prefix(prefix);
        all.insert(all.end(), results.begin(), results.end());
    }
    return all;
}

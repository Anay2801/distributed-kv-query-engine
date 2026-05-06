#include "consistent_hash_ring.hpp"
#include "wal_checksum.hpp"

ConsistentHashRing::ConsistentHashRing(int vnodes_per_node)
    : vnodes_per_node_(vnodes_per_node) {}

void ConsistentHashRing::add_node(const std::string& node_id) {
    for (int i = 0; i < vnodes_per_node_; ++i) {
        uint32_t h = wal_checksum(node_id + "#" + std::to_string(i));
        ring_[h] = node_id;
    }
}

void ConsistentHashRing::remove_node(const std::string& node_id) {
    for (int i = 0; i < vnodes_per_node_; ++i) {
        uint32_t h = wal_checksum(node_id + "#" + std::to_string(i));
        ring_.erase(h);
    }
}

std::string ConsistentHashRing::get_node(const std::string& key) const {
    if (ring_.empty()) return "";
    uint32_t h = wal_checksum(key);
    auto it = ring_.lower_bound(h);
    if (it == ring_.end()) it = ring_.begin();  // wrap around
    return it->second;
}

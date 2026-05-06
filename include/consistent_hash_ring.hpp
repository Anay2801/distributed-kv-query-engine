#pragma once
#include <cstdint>
#include <map>
#include <string>

class ConsistentHashRing {
public:
    explicit ConsistentHashRing(int vnodes_per_node = 150);
    void add_node(const std::string& node_id);
    void remove_node(const std::string& node_id);
    std::string get_node(const std::string& key) const;

private:
    std::map<uint32_t, std::string> ring_;
    int vnodes_per_node_;
};

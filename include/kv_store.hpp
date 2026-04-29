#pragma once
#include "kv_filter.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class KVStore {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    std::vector<std::pair<std::string, std::string>> scan(const KVPredicate& predicate) const;

private:
    std::unordered_map<std::string, std::string> data_;
};

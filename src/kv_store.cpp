#include "kv_store.hpp"

void KVStore::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::optional<std::string> KVStore::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

bool KVStore::del(const std::string& key) {
    return data_.erase(key) > 0;
}

std::vector<std::pair<std::string, std::string>> KVStore::scan(const KVPredicate& predicate) const {
    std::vector<std::pair<std::string, std::string>> results;
    for (const auto& [key, value] : data_) {
        if (predicate(key, value)) {
            results.emplace_back(key, value);
        }
    }
    return results;
}

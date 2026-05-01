#pragma once
#include "kv_store.hpp"
#include <fstream>
#include <string>

class LoggedKVStore {
public:
    explicit LoggedKVStore(const std::string& log_path);
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    std::vector<std::pair<std::string, std::string>> scan(const KVPredicate& predicate) const;

private:
    void replay(const std::string& log_path);
    KVStore store_;
    std::ofstream log_;
};

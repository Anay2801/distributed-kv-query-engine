#pragma once
#include "kv_store.hpp"
#include <cstdint>
#include <string>

class WALStore {
public:
    explicit WALStore(const std::string& log_path);
    ~WALStore();
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    std::vector<std::pair<std::string, std::string>> scan(const KVPredicate& predicate) const;

private:
    void replay(const std::string& log_path);
    std::string format_entry(const std::string& op, const std::string& key,
                              const std::string& value) const;
    void append_and_sync(const std::string& entry);

    KVStore store_;
    int fd_{-1};
    uint64_t next_lsn_{0};
};

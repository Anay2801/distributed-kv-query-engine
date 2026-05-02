#include "wal_store.hpp"
#include "wal_checksum.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

WALStore::WALStore(const std::string& log_path) {
    replay(log_path);
    fd_ = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
}

WALStore::~WALStore() {
    if (fd_ >= 0) ::close(fd_);
}

void WALStore::replay(const std::string& log_path) {
    std::ifstream in(log_path);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        auto last_space = line.rfind(' ');
        if (last_space == std::string::npos) continue;

        std::string content = line.substr(0, last_space);
        std::string checksum_str = line.substr(last_space + 1);

        uint32_t actual;
        try {
            actual = static_cast<uint32_t>(std::stoul(checksum_str));
        } catch (...) { continue; }

        if (wal_checksum(content) != actual) continue;

        std::istringstream ss(content);
        uint64_t lsn;
        std::string op, key, value;
        if (!(ss >> lsn >> op >> key)) continue;
        std::getline(ss, value);
        if (!value.empty() && value[0] == ' ') value = value.substr(1);

        if (lsn >= next_lsn_) next_lsn_ = lsn + 1;

        if (op == "SET") {
            store_.set(key, value);
        } else if (op == "DEL") {
            store_.del(key);
        }
    }
}

std::string WALStore::format_entry(const std::string& op, const std::string& key,
                                    const std::string& value) const {
    std::string content = std::to_string(next_lsn_) + " " + op + " " + key;
    if (!value.empty()) content += " " + value;
    uint32_t checksum = wal_checksum(content);
    return content + " " + std::to_string(checksum) + "\n";
}

void WALStore::append_and_sync(const std::string& entry) {
    ::write(fd_, entry.c_str(), entry.size());
    ::fsync(fd_);
}

void WALStore::set(const std::string& key, const std::string& value) {
    std::string entry = format_entry("SET", key, value);
    append_and_sync(entry);   // 1. Write log + fsync (write-ahead)
    ++next_lsn_;
    store_.set(key, value);   // 2. Apply to memory
}

std::optional<std::string> WALStore::get(const std::string& key) const {
    return store_.get(key);
}

bool WALStore::del(const std::string& key) {
    if (!store_.get(key).has_value()) return false;
    std::string entry = format_entry("DEL", key, "");
    append_and_sync(entry);   // 1. Write log + fsync (write-ahead)
    ++next_lsn_;
    store_.del(key);          // 2. Apply to memory
    return true;
}

std::vector<std::pair<std::string, std::string>> WALStore::scan(const KVPredicate& predicate) const {
    return store_.scan(predicate);
}

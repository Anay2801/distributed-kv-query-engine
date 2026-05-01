#include "logged_kv_store.hpp"
#include <fstream>

LoggedKVStore::LoggedKVStore(const std::string& log_path) {
    replay(log_path);
    log_.open(log_path, std::ios::app);
}

void LoggedKVStore::replay(const std::string& log_path) {
    std::ifstream in(log_path);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.size() > 4 && line.substr(0, 4) == "SET ") {
            auto space = line.find(' ', 4);
            if (space == std::string::npos) continue;
            store_.set(line.substr(4, space - 4), line.substr(space + 1));
        } else if (line.size() > 4 && line.substr(0, 4) == "DEL ") {
            store_.del(line.substr(4));
        }
    }
}

void LoggedKVStore::set(const std::string& key, const std::string& value) {
    store_.set(key, value);
    log_ << "SET " << key << " " << value << "\n";
    log_.flush();
}

std::optional<std::string> LoggedKVStore::get(const std::string& key) const {
    return store_.get(key);
}

bool LoggedKVStore::del(const std::string& key) {
    bool existed = store_.del(key);
    if (existed) {
        log_ << "DEL " << key << "\n";
        log_.flush();
    }
    return existed;
}

std::vector<std::pair<std::string, std::string>> LoggedKVStore::scan(const KVPredicate& predicate) const {
    return store_.scan(predicate);
}

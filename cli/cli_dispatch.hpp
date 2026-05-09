#pragma once
#include "kv_filter.hpp"
#include "wal_store.hpp"
#include <algorithm>
#include <string>
#include <vector>

inline std::string run_command(WALStore& store, const std::vector<std::string>& args) {
    if (args.empty()) return "error: no command\n";
    const std::string& cmd = args[0];

    if (cmd == "set") {
        if (args.size() < 3) return "error: set requires <key> <value>\n";
        std::string value;
        for (size_t i = 2; i < args.size(); ++i) {
            if (i > 2) value += ' ';
            value += args[i];
        }
        store.set(args[1], value);
        return "OK\n";
    }

    if (cmd == "get") {
        if (args.size() < 2) return "error: get requires <key>\n";
        auto val = store.get(args[1]);
        return val ? (*val + "\n") : "(nil)\n";
    }

    if (cmd == "del") {
        if (args.size() < 2) return "error: del requires <key>\n";
        return store.del(args[1]) ? "OK\n" : "(nil)\n";
    }

    if (cmd == "scan") {
        if (args.size() < 2) return "error: scan requires <prefix>\n";
        auto results = store.scan(filter::key_prefix(args[1]));
        if (results.empty()) return "(empty)\n";
        std::sort(results.begin(), results.end());
        std::string out;
        for (const auto& [k, v] : results) out += k + " " + v + "\n";
        return out;
    }

    return "error: unknown command: " + cmd + "\n";
}

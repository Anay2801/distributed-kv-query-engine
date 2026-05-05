#pragma once
#include <functional>
#include <string>

using KVPredicate = std::function<bool(const std::string&, const std::string&)>;

namespace filter {
    inline KVPredicate key_prefix(const std::string& prefix) {
        return [prefix](const std::string& k, const std::string&) {
            return k.compare(0, prefix.size(), prefix) == 0;
        };
    }

    inline KVPredicate key_contains(const std::string& substr) {
        return [substr](const std::string& k, const std::string&) {
            return k.find(substr) != std::string::npos;
        };
    }

    inline KVPredicate value_equals(const std::string& val) {
        return [val](const std::string&, const std::string& v) {
            return v == val;
        };
    }

    inline KVPredicate value_prefix(const std::string& prefix) {
        return [prefix](const std::string&, const std::string& v) {
            return v.compare(0, prefix.size(), prefix) == 0;
        };
    }
}

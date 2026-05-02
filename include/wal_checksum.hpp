#pragma once
#include <cstdint>
#include <string>

inline uint32_t wal_checksum(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

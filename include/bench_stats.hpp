#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

struct BenchStats {
    double mean_ns{0};
    uint64_t p50_ns{0};
    uint64_t p99_ns{0};
    double throughput_ops_sec{0};
    uint64_t total_ops{0};
    double elapsed_sec{0};
};

inline BenchStats compute_stats(const std::vector<uint64_t>& samples, double elapsed_sec) {
    BenchStats s;
    s.total_ops = samples.size();
    s.elapsed_sec = elapsed_sec;
    if (samples.empty()) return s;

    s.throughput_ops_sec = static_cast<double>(s.total_ops) / elapsed_sec;

    std::vector<uint64_t> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    uint64_t sum = 0;
    for (auto v : sorted) sum += v;
    s.mean_ns = static_cast<double>(sum) / static_cast<double>(sorted.size());

    s.p50_ns = sorted[sorted.size() * 50 / 100];
    s.p99_ns = sorted[sorted.size() * 99 / 100];

    return s;
}

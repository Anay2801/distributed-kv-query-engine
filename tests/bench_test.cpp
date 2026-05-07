#include <gtest/gtest.h>
#include "bench_stats.hpp"

TEST(BenchStatsTest, MeanIsCorrect) {
    std::vector<uint64_t> samples = {100, 200, 300};
    auto stats = compute_stats(samples, 1.0);
    EXPECT_DOUBLE_EQ(stats.mean_ns, 200.0);
}

TEST(BenchStatsTest, P50UsesFloorHalfIndex) {
    // 10 samples sorted; p50 index = 10*50/100 = 5 → sorted[5] = 600
    std::vector<uint64_t> samples = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    auto stats = compute_stats(samples, 1.0);
    EXPECT_EQ(stats.p50_ns, 600u);
}

TEST(BenchStatsTest, P99UsesNearEndIndex) {
    // 100 samples 1..100; p99 index = 100*99/100 = 99 → sorted[99] = 100
    std::vector<uint64_t> samples(100);
    for (uint64_t i = 0; i < 100; ++i) samples[i] = i + 1;
    auto stats = compute_stats(samples, 1.0);
    EXPECT_EQ(stats.p99_ns, 100u);
}

TEST(BenchStatsTest, ThroughputIsOpsPerSec) {
    std::vector<uint64_t> samples(1000, 5000u);  // 1000 ops
    auto stats = compute_stats(samples, 0.5);     // 0.5 seconds
    EXPECT_DOUBLE_EQ(stats.throughput_ops_sec, 2000.0);
}

TEST(BenchStatsTest, TotalOpsMatchesSampleCount) {
    std::vector<uint64_t> samples(42, 100u);
    auto stats = compute_stats(samples, 1.0);
    EXPECT_EQ(stats.total_ops, 42u);
}

TEST(BenchStatsTest, EmptySamplesReturnsZero) {
    auto stats = compute_stats({}, 1.0);
    EXPECT_EQ(stats.total_ops, 0u);
    EXPECT_DOUBLE_EQ(stats.mean_ns, 0.0);
    EXPECT_EQ(stats.p50_ns, 0u);
    EXPECT_EQ(stats.p99_ns, 0u);
    EXPECT_DOUBLE_EQ(stats.throughput_ops_sec, 0.0);
}

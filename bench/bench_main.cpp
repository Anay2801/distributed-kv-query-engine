#include "bench_stats.hpp"
#include "node_server.hpp"
#include "shard_router.hpp"
#include "wal_store.hpp"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

static constexpr int N_OPS    = 5000;
static constexpr int N_SHARDS = 3;

template <typename Store>
BenchStats run_write(Store& store, int n) {
    std::vector<uint64_t> samples;
    samples.reserve(n);
    auto wall_start = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        auto s = std::chrono::steady_clock::now();
        store.set("key_" + std::to_string(i), "val_" + std::to_string(i));
        auto e = std::chrono::steady_clock::now();
        samples.push_back(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count()));
    }
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();
    return compute_stats(samples, elapsed);
}

template <typename Store>
BenchStats run_read(Store& store, int n) {
    for (int i = 0; i < n; ++i)
        store.set("key_" + std::to_string(i), "val_" + std::to_string(i));
    std::vector<uint64_t> samples;
    samples.reserve(n);
    auto wall_start = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        auto s = std::chrono::steady_clock::now();
        store.get("key_" + std::to_string(i));
        auto e = std::chrono::steady_clock::now();
        samples.push_back(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count()));
    }
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();
    return compute_stats(samples, elapsed);
}

template <typename Store>
BenchStats run_mixed(Store& store, int n) {
    for (int i = 0; i < n; ++i)
        store.set("key_" + std::to_string(i), "val_" + std::to_string(i));
    std::vector<uint64_t> samples;
    samples.reserve(n);
    auto wall_start = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        std::string k = "key_" + std::to_string(i % n);
        auto s = std::chrono::steady_clock::now();
        if (i % 2 == 0)
            store.get(k);
        else
            store.set(k, "upd_" + std::to_string(i));
        auto e = std::chrono::steady_clock::now();
        samples.push_back(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count()));
    }
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();
    return compute_stats(samples, elapsed);
}

struct Row {
    std::string backend;
    std::string workload;
    BenchStats  stats;
};

static void write_csv(const std::vector<Row>& rows, const std::string& path) {
    std::ofstream f(path);
    f << "backend,workload,n_ops,throughput_ops_sec,mean_us,p50_us,p99_us\n";
    for (const auto& r : rows) {
        f << r.backend    << ","
          << r.workload   << ","
          << r.stats.total_ops << ","
          << r.stats.throughput_ops_sec << ","
          << (r.stats.mean_ns / 1000.0) << ","
          << (r.stats.p50_ns  / 1000.0) << ","
          << (r.stats.p99_ns  / 1000.0) << "\n";
    }
}

static void print_results(const std::vector<Row>& rows) {
    std::printf("\n%-15s %-8s %7s %14s %10s %10s %10s\n",
                "Backend", "Workload", "Ops", "Ops/sec", "Mean(us)", "P50(us)", "P99(us)");
    std::printf("%s\n", std::string(79, '-').c_str());
    for (const auto& r : rows) {
        std::printf("%-15s %-8s %7llu %14.1f %10.1f %10.1f %10.1f\n",
                    r.backend.c_str(),
                    r.workload.c_str(),
                    static_cast<unsigned long long>(r.stats.total_ops),
                    r.stats.throughput_ops_sec,
                    r.stats.mean_ns / 1000.0,
                    r.stats.p50_ns  / 1000.0,
                    r.stats.p99_ns  / 1000.0);
    }
}

int main() {
    std::vector<Row> rows;

    // WALStore benchmarks — fresh log file per workload
    auto wal_bench = [&](const std::string& workload) {
        std::string log = "/tmp/bench_wal_" + workload + ".log";
        std::remove(log.c_str());
        WALStore store(log);
        BenchStats stats{};
        if      (workload == "write") stats = run_write(store, N_OPS);
        else if (workload == "read")  stats = run_read(store,  N_OPS);
        else if (workload == "mixed") stats = run_mixed(store, N_OPS);
        std::remove(log.c_str());
        rows.push_back({"WALStore", workload, stats});
    };
    wal_bench("write");
    wal_bench("read");
    wal_bench("mixed");

    // ShardRouter benchmarks — fresh servers per workload
    auto shard_bench = [&](const std::string& workload) {
        std::vector<std::unique_ptr<NodeServer>> servers;
        std::vector<std::string> log_paths;
        for (int i = 0; i < N_SHARDS; ++i) {
            std::string lp = "/tmp/bench_shard_" + workload + "_" + std::to_string(i) + ".log";
            std::remove(lp.c_str());
            log_paths.push_back(lp);
            servers.push_back(std::make_unique<NodeServer>(0, lp));
            servers.back()->start();
        }
        std::map<std::string, std::pair<std::string, int>> endpoints;
        for (int i = 0; i < N_SHARDS; ++i)
            endpoints["node" + std::to_string(i)] = {"127.0.0.1", servers[i]->port()};
        {
            ShardRouter router(endpoints);
            BenchStats stats{};
            if      (workload == "write") stats = run_write(router, N_OPS);
            else if (workload == "read")  stats = run_read(router,  N_OPS);
            else if (workload == "mixed") stats = run_mixed(router, N_OPS);
            rows.push_back({"ShardRouter", workload, stats});
        }  // router destroyed here — NodeClients disconnect cleanly
        for (auto& srv : servers) srv->stop();
        for (const auto& lp : log_paths) std::remove(lp.c_str());
    };
    shard_bench("write");
    shard_bench("read");
    shard_bench("mixed");

    const std::string csv_path = "benchmark_results.csv";
    write_csv(rows, csv_path);
    print_results(rows);
    std::printf("\nCSV written to: %s\n", csv_path.c_str());
    return 0;
}

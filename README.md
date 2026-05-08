# Distributed KV Query Engine

A layered, distributed key-value store in C++17. Starts from a pure in-memory hash map and builds up through disk persistence, write-ahead logging, a filter-based query layer, consistent-hash sharding across TCP nodes, and a benchmarking harness.

## Architecture

| Layer | Class | What it does |
|-------|-------|--------------|
| In-memory store | `KVStore` | `std::unordered_map`; O(1) get/set/del |
| Disk persistence | `LoggedKVStore` | Append-only log; replays on open to rebuild index |
| Write-ahead log | `WALStore` | Log-first + `fsync` before memory apply; FNV-1a checksums; crash-safe replay |
| Query layer | `KVStore::scan` | Predicate-based scan; `filter::` namespace + custom lambdas |
| Hash ring | `ConsistentHashRing` | 150 virtual nodes per real node; FNV-1a; O(log N) lookup |
| TCP node | `NodeServer` / `NodeClient` | Newline-delimited text protocol (SET/GET/DEL/SCAN_PREFIX) |
| Shard router | `ShardRouter` | Routes single-key ops to one shard; `scan_prefix` fans out to all |
| Benchmarks | `kv_bench` | Per-op latency sampling; throughput + p50/p99; CSV output |

## Prerequisites

- CMake 3.20+
- C++17 compiler (clang++ or g++)
- macOS or Linux (uses POSIX sockets and `fsync`)

```bash
# macOS
brew install cmake
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

Expected: `100% tests passed, 0 tests failed out of 49`

## Usage

### In-memory store

```cpp
#include "kv_store.hpp"

KVStore store;
store.set("user:1", "alice");
auto val = store.get("user:1");  // std::optional<std::string> → "alice"
store.del("user:1");
```

### Persistent store (WAL)

```cpp
#include "wal_store.hpp"

WALStore store("/tmp/mydb.wal");  // replays log on open; survives crashes
store.set("key", "value");        // writes + fsyncs log before returning
auto val = store.get("key");      // std::optional<std::string>
store.del("key");
```

Data survives process restarts: construct a new `WALStore` with the same path and all committed writes are replayed automatically.

### Filter queries

```cpp
#include "kv_filter.hpp"
#include "wal_store.hpp"

WALStore store("/tmp/mydb.wal");
store.set("user:1", "alice");
store.set("user:2", "bob");
store.set("item:1", "widget");

// Built-in predicates
auto users   = store.scan(filter::key_prefix("user:"));
auto bobs    = store.scan(filter::value_equals("bob"));
auto widgets = store.scan(filter::value_prefix("wid"));
auto tagged  = store.scan(filter::key_contains("item"));

// Ad-hoc lambda — combine conditions freely
auto custom = store.scan([](const std::string& k, const std::string& v) {
    return k.size() > 5 && v != "deleted";
});
```

`scan` returns `std::vector<std::pair<std::string,std::string>>`. Results are unordered; sort if you need a stable order.

### Sharded cluster

```cpp
#include "node_server.hpp"
#include "shard_router.hpp"

// Start nodes (port 0 → OS assigns an ephemeral port)
NodeServer n0(0, "/tmp/shard0.wal");
NodeServer n1(0, "/tmp/shard1.wal");
NodeServer n2(0, "/tmp/shard2.wal");
n0.start(); n1.start(); n2.start();

// Build the router — keys are distributed via consistent hash ring
std::map<std::string, std::pair<std::string, int>> endpoints = {
    {"node0", {"127.0.0.1", n0.port()}},
    {"node1", {"127.0.0.1", n1.port()}},
    {"node2", {"127.0.0.1", n2.port()}},
};
ShardRouter router(endpoints);

router.set("key", "value");
auto val     = router.get("key");            // routed to one shard
auto results = router.scan_prefix("user:");  // fans out to all shards, merges

n0.stop(); n1.stop(); n2.stop();
```

Adding a node requires constructing a new `ConsistentHashRing` with the updated node set; data rebalancing is not automatic in this implementation.

### Benchmark

```bash
./build/bench/kv_bench
```

Runs write, read, and mixed workloads against `WALStore` (local) and `ShardRouter` (3 in-process shards), then prints a summary table and writes `benchmark_results.csv`:

```
backend,workload,n_ops,throughput_ops_sec,mean_us,p50_us,p99_us
WALStore,write,5000,...
WALStore,read,5000,...
...
```

### CLI (kv_cli)

```bash
./build/cli/kv_cli set user:1 alice
./build/cli/kv_cli get user:1
./build/cli/kv_cli del user:1
./build/cli/kv_cli scan user:
```

Data is stored in `~/.kvdb/data.wal` by default. Override with `--db`:

```bash
./build/cli/kv_cli --db /tmp/mydb.wal set key value
./build/cli/kv_cli --db /tmp/mydb.wal get key
```

Commands:

| Command | Args | Output |
|---------|------|--------|
| `set` | `<key> <value...>` | `OK` — value may be multiple words |
| `get` | `<key>` | value, or `(nil)` if absent |
| `del` | `<key>` | `OK` if deleted, `(nil)` if absent |
| `scan` | `<prefix>` | `key value` lines sorted by key, or `(empty)` |

Data persists across invocations — each `kv_cli` call opens the WAL, replays it to rebuild the in-memory index, and closes on exit.

## WAL Log Format

Each entry on one line: `<lsn> <op> <key> [<value>] <fnv1a_checksum>`

```
0 SET mykey hello world 3141592653
1 SET counter 42 2718281828
2 DEL mykey 1234567890
```

- LSN is monotonically increasing; restored on replay so new entries always get higher numbers.
- The checksum covers everything before it on the line. Corrupted or truncated entries are silently skipped on replay.

## Directory Structure

```
.
├── include/                  # Public headers (all header-only or forward declarations)
│   ├── kv_store.hpp
│   ├── logged_kv_store.hpp
│   ├── wal_store.hpp
│   ├── kv_filter.hpp         # KVPredicate type + filter:: factory functions
│   ├── wal_checksum.hpp      # FNV-1a hash (shared by WAL and hash ring)
│   ├── consistent_hash_ring.hpp
│   ├── node_server.hpp
│   ├── node_client.hpp
│   ├── shard_router.hpp
│   └── bench_stats.hpp       # BenchStats struct + compute_stats()
├── src/                      # Implementations
├── tests/                    # Google Test suites (58 tests)
├── bench/                    # kv_bench standalone binary
├── cli/                      # kv_cli terminal tool
└── tasks/                    # progress.md + lessons.md
```

## Design Notes

- **Composition over inheritance.** `LoggedKVStore` and `WALStore` each embed a `KVStore` — they don't extend it. Changing one store does not affect the other's tests.
- **Write-ahead means log-first.** `WALStore` writes and `fsync`s the log entry *before* modifying the in-memory map. A crash after `fsync` but before the memory apply is safe — replay re-applies the entry on next open.
- **`fsync` ≠ `flush`.** `std::ofstream::flush()` moves bytes from the C++ buffer to the OS page cache. Only `fsync()` pushes them to durable storage. The WAL uses raw POSIX `open`/`write`/`fsync`.
- **Virtual nodes smooth distribution.** Each real node gets 150 virtual entries on the hash ring, so adding or removing a node only reshuffles ~1/N of the keys rather than causing a full remap.
- **`scan_prefix` must fan out.** Because keys are distributed across shards, there is no way to know which shard holds keys matching a prefix without querying every shard.

# Progress

## Step 1: Project Scaffold ✅
- CMakeLists.txt configured for C++17
- Google Test 1.14.0 integrated via FetchContent
- Smoke test passes: `SmokeTest.BuildPipelineWorks`
- `src/` and `tests/` subdirectory structure established
- `include/` directory created for future headers

## Step 2: In-Memory KV Store ✅
- `KVStore` class: `set`, `get` (returns `std::optional`), `del`
- Backed by `std::unordered_map<std::string, std::string>`
- `kv_engine_lib` converted from INTERFACE to STATIC library
- 5 unit tests pass (+ 1 smoke = 6 total)

## Step 3: Disk Persistence ✅
- `LoggedKVStore` wraps `KVStore`, appends mutations to flat log file
- Log format: `SET key value` / `DEL key` (newline-delimited)
- Startup replays log to rebuild in-memory index
- `KVStore` untouched — all Step 2 tests still pass
- 7 new persistence tests (13 total)

## Step 4: Write-Ahead Log (WAL) ✅
- `WALStore` composes `KVStore`; log-first + fsync before any memory write
- Log format: `<lsn> <op> <key> [<value>] <checksum>` (FNV-1a checksum is last token)
- Replay verifies each entry's checksum; corrupted/truncated entries silently skipped
- `next_lsn_` restored from log on replay so new entries always get higher LSN
- POSIX `open`/`write`/`fsync` used directly — `std::ofstream::flush()` only clears C++ buffer, not disk
- 9 new WAL tests (22 total); all Step 2 and Step 3 tests still pass

## Step 5: Query Layer ✅
- `scan(KVPredicate)` added to `KVStore`, `LoggedKVStore`, and `WALStore`
- `KVPredicate` = `std::function<bool(const std::string& key, const std::string& value)>`
- `filter::` namespace provides: `key_prefix`, `key_contains`, `value_equals`, `value_prefix`
- Caller-supplied lambdas also accepted for ad-hoc or combined predicates
- `LoggedKVStore::scan` and `WALStore::scan` delegate to their embedded `KVStore::scan`
- Results are unordered (reflect `unordered_map` iteration); callers sort if needed
- 9 new query tests (31 total)

## Step 6: Sharding ✅
- `ConsistentHashRing`: virtual nodes on a sorted `std::map<uint32_t, node_id>`; reuses FNV-1a from `wal_checksum.hpp`
- `NodeServer`: POSIX TCP server wrapping `WALStore`; port 0 → OS-assigned; accept thread; `stop()` closes listen socket to unblock `accept()`
- `NodeClient`: POSIX TCP client; newline-delimited text protocol (SET/GET/DEL/SCAN_PREFIX)
- `ShardRouter`: routes single-key ops to one shard via ring; `scan_prefix` fans out to all shards and merges
- 12 new shard tests (43 total)

## Step 7: Benchmarking Harness ✅
- `bench_stats.hpp`: header-only `BenchStats` struct + `compute_stats()` (mean, p50, p99, throughput)
- `kv_bench` standalone binary: write/read/mixed workloads on `WALStore` and `ShardRouter` (3 shards)
- Per-op latency sampled with `std::chrono::steady_clock`; results in µs
- CSV output: `benchmark_results.csv` (backend, workload, n_ops, throughput, mean, p50, p99)
- 6 new stats tests (49 total); `kv_bench` is not a ctest target

## Step 8: README + Cleanup ✅
- `README.md` written: architecture table, build/test instructions, usage examples for all layers
- `.gitignore` updated: `benchmark_results.csv` excluded
- `include/.gitkeep` removed (directory has real headers)
- `tasks/progress.md` ordering fixed: Step 6 now precedes Step 7
- `CLAUDE.md` updated: all 8 build-order steps marked complete

## kv_cli: Terminal Interface ✅
- `cli_dispatch.hpp`: header-only `run_command(WALStore&, vector<string>) -> string`; handles set/get/del/scan; multi-word values joined from trailing args; scan output sorted for determinism
- `kv_cli` binary: `--db <path>` flag overrides default `~/.kvdb/data.wal`; `std::filesystem::create_directories` ensures parent dir exists
- 9 new CLI tests (58 total); `kv_cli` is not a ctest target

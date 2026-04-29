# Lessons

## Step 1
- FetchContent with a URL tag is simpler than find_package for a self-contained repo — no system gtest required.
- `DOWNLOAD_EXTRACT_TIMESTAMP TRUE` suppresses a CMake 3.24+ warning about archive timestamps.
- `gtest_discover_tests` is preferred over `add_test` — it discovers individual TEST() cases so ctest reports per-test results.
- `GTest::gtest_main` supplies main(); no need to write your own.
- `kv_engine_lib` as an INTERFACE library now means no source files needed yet — test target links cleanly.
- CMake is not installed by default on macOS — `brew install cmake` required before first configure.

## Step 2
- `std::optional<std::string>` (C++17) is the right return type for get() — avoids sentinel values like empty string.
- Converting INTERFACE → STATIC requires changing `target_include_directories` scope from INTERFACE to PUBLIC so downstream targets (tests) still get the include path.
- `data_.erase(key)` returns the number of elements removed — comparing to `> 0` gives a clean bool for del().
- In C++ TDD, the two-stage red (compile error → linker error → green) maps to the classic red/green cycle.

## Step 3
- Composition over inheritance: `LoggedKVStore` has-a `KVStore` rather than extending it — keeps `KVStore` pure and Step 2 tests untouched.
- `std::ofstream` opened with `std::ios::app` creates the file if absent and seeks to end — no separate "create" step needed.
- Replay opens a separate `std::ifstream`; the write-path `std::ofstream` is opened after replay completes so the two file handles don't interfere.
- `log_.flush()` after each write ensures the entry is visible to a reader before the next operation — important for correctness even before WAL (Step 4).
- Values as "everything after the second space on a SET line" allows space-containing values without a more complex encoding.

## Step 4
- Write-ahead means log entry is durably on disk BEFORE memory is mutated — crash after fsync but before memory apply is safe because replay re-applies the entry.
- `std::ofstream::flush()` (Step 3) only moves bytes from the C++ stream buffer to the OS page cache; `fsync()` is required to push to durable storage.
- POSIX `open()`/`write()`/`fsync()` is simpler than extracting a file descriptor from `std::ofstream` for the purpose of calling `fsync`.
- Checksum-at-end format lets values contain spaces cleanly: `rfind(' ')` splits off the last token, everything before it is the content.
- Extracting `wal_checksum` into a header-only `wal_checksum.hpp` lets tests construct valid WAL entries to simulate crash scenarios without coupling to implementation internals.
- A read check before writing a DEL entry (`store_.get(key).has_value()`) avoids writing unnecessary log entries for no-op deletes, while remaining crash-safe (no mutation occurred before the WAL write).

## Step 5
- `std::function<bool(const std::string&, const std::string&)>` as a predicate type is the right tradeoff for a learning project — accepts lambdas, function pointers, and named function objects without any virtual dispatch boilerplate.
- `std::string::compare(pos, len, other)` avoids the allocation that `substr(0, n) == other` would cause; it also handles the case where the string is shorter than the prefix (returns non-zero without UB).
- `std::unordered_map` has no defined iteration order — tests must sort results before asserting on specific positions.
- Structured bindings (`const auto& [key, value]`) in the range-for loop over `data_` (C++17) make the scan implementation read cleanly.
- Keeping `scan` as a forwarding method on `LoggedKVStore` and `WALStore` (one-liners delegating to the embedded `KVStore`) avoids duplicating iteration logic while keeping all three stores' interfaces consistent.

## Step 7
- `std::chrono::steady_clock` is the correct clock for benchmarking — it is monotonic (never goes backwards), unlike `system_clock` which can be adjusted by NTP.
- Pre-allocating the latency sample vector with `reserve(n)` avoids heap allocations in the hot measurement loop, preventing allocation jitter from skewing p99.
- Percentiles from a sorted vector: p50 at `sorted[n * 50 / 100]`, p99 at `sorted[n * 99 / 100]` (integer division gives floor index) — not the same as the statistical median but standard practice for benchmark reporting.
- The benchmark binary is a separate CMake target (`add_executable(kv_bench ...)`) under `bench/` with its own `CMakeLists.txt` and `add_subdirectory(bench)` in the root — it does not register with `ctest`, keeping the test suite fast and deterministic.
- Destroying `ShardRouter` before stopping `NodeServer`s is the correct teardown order: clients disconnect cleanly (closing TCP sockets), then `stop()` on each server unblocks the `accept()` call.

## Step 6
- Consistent hash ring uses `std::map<uint32_t, node_id>` — `lower_bound(hash(key))` with wrap-around gives O(log N) lookup; virtual nodes smooth key distribution.
- Passing port 0 to `bind()` lets the OS assign an ephemeral port; `getsockname()` retrieves it — essential for test isolation (no hardcoded ports that could conflict).
- `SO_REUSEADDR` on the listen socket prevents "Address already in use" errors when tests restart servers quickly.
- Closing `listen_fd_` in `stop()` is the simplest way to unblock a blocking `accept()` call in another thread — `accept()` returns -1 and the loop exits.
- `std::thread` requires linking with `Threads::Threads` in CMake; `find_package(Threads REQUIRED)` + `target_link_libraries(... Threads::Threads)` is the portable way to add `-pthread`.
- CMake will error at configure time if source files listed in `add_library` don't yet exist on disk — stub or real files must be present before `cmake -S . -B build` runs.
- `ShardRouter::scan_prefix` must fan out to ALL shards because keys are distributed — there is no way to know which shard holds keys matching a prefix without asking every shard.

## Step 8
- Generated output files (`benchmark_results.csv`) should be in `.gitignore` from day one — they are artifacts, not source.
- A `README.md` usage example is only trustworthy if you run the exact commands it shows; copy-paste the build and test commands from the README into the shell to verify them rather than assuming they are correct.
- `.gitkeep` files are a common convention for tracking empty directories in git; remove them once real files exist so they don't confuse readers of `git ls-files`.

## kv_cli
- Extract command dispatch into a header-only function (`run_command`) to keep it unit-testable without forking a subprocess — the same pattern as `bench_stats.hpp`.
- `std::filesystem::create_directories` (C++17) is the simplest way to ensure `~/.kvdb/` exists before constructing `WALStore`; no need for POSIX `mkdir` calls.
- Joining trailing argv tokens as the value (`args[2..n]`) lets callers omit quotes for multi-word values (`kv_cli set msg hello world`) — consistent with how the WAL log format already supports space-containing values.
- `scan` output must be sorted before printing — `std::unordered_map` iteration order is undefined, and unsorted output makes scripting fragile.

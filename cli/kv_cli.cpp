#include "cli_dispatch.hpp"
#include "wal_store.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::string db_path;
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        } else {
            args.push_back(arg);
        }
    }

    if (db_path.empty()) {
        const char* home = std::getenv("HOME");
        db_path = std::string(home ? home : "/tmp") + "/.kvdb/data.wal";
    }

    std::filesystem::create_directories(std::filesystem::path(db_path).parent_path());

    WALStore store(db_path);
    std::cout << run_command(store, args);
    return 0;
}

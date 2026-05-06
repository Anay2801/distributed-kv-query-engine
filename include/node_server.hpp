#pragma once
#include "wal_store.hpp"
#include <atomic>
#include <string>
#include <thread>

class NodeServer {
public:
    NodeServer(int port, const std::string& log_path);
    ~NodeServer();
    void start();
    void stop();
    int port() const { return port_; }

private:
    void accept_loop();
    void serve_connection(int client_fd);
    std::string handle_command(const std::string& line);

    WALStore store_;
    int listen_fd_{-1};
    int port_{0};
    std::atomic<bool> running_{false};
    std::thread thread_;
};

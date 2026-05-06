#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>

class NodeClient {
public:
    NodeClient(const std::string& host, int port);
    ~NodeClient();
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    std::vector<std::pair<std::string, std::string>> scan_prefix(const std::string& prefix);

private:
    int sock_fd_{-1};
    std::string read_line();
    void send_line(const std::string& line);
};

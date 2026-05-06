#include "node_client.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

NodeClient::NodeClient(const std::string& host, int port) {
    sock_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    ::connect(sock_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
}

NodeClient::~NodeClient() {
    if (sock_fd_ >= 0) ::close(sock_fd_);
}

void NodeClient::send_line(const std::string& line) {
    std::string msg = line + "\n";
    ::write(sock_fd_, msg.c_str(), msg.size());
}

std::string NodeClient::read_line() {
    std::string line;
    char c;
    while (::read(sock_fd_, &c, 1) == 1 && c != '\n') {
        line += c;
    }
    return line;
}

void NodeClient::set(const std::string& key, const std::string& value) {
    send_line("SET " + key + " " + value);
    read_line();  // consume "OK"
}

std::optional<std::string> NodeClient::get(const std::string& key) {
    send_line("GET " + key);
    std::string resp = read_line();
    if (resp.size() > 6 && resp.substr(0, 6) == "VALUE ") return resp.substr(6);
    return std::nullopt;
}

bool NodeClient::del(const std::string& key) {
    send_line("DEL " + key);
    return read_line() == "OK";
}

std::vector<std::pair<std::string, std::string>> NodeClient::scan_prefix(const std::string& prefix) {
    send_line("SCAN_PREFIX " + prefix);
    std::vector<std::pair<std::string, std::string>> results;
    std::string line;
    while ((line = read_line()) != "END") {
        if (line.size() > 7 && line.substr(0, 7) == "RESULT ") {
            std::string rest = line.substr(7);
            auto sp = rest.find(' ');
            if (sp != std::string::npos) {
                results.emplace_back(rest.substr(0, sp), rest.substr(sp + 1));
            }
        }
    }
    return results;
}

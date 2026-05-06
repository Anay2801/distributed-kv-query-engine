#include "node_server.hpp"
#include "kv_filter.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

NodeServer::NodeServer(int port, const std::string& log_path)
    : store_(log_path) {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    ::bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    ::listen(listen_fd_, 10);

    socklen_t len = sizeof(addr);
    ::getsockname(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), &len);
    port_ = ntohs(addr.sin_port);
}

NodeServer::~NodeServer() {
    if (running_) stop();
}

void NodeServer::start() {
    running_ = true;
    thread_ = std::thread(&NodeServer::accept_loop, this);
}

void NodeServer::stop() {
    running_ = false;
    ::close(listen_fd_);
    listen_fd_ = -1;
    if (thread_.joinable()) thread_.join();
}

void NodeServer::accept_loop() {
    while (running_) {
        int client_fd = ::accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0) break;
        serve_connection(client_fd);
    }
}

void NodeServer::serve_connection(int client_fd) {
    std::string buf;
    char c;
    while (::read(client_fd, &c, 1) == 1) {
        if (c == '\n') {
            std::string response = handle_command(buf);
            ::write(client_fd, response.c_str(), response.size());
            buf.clear();
        } else {
            buf += c;
        }
    }
    ::close(client_fd);
}

std::string NodeServer::handle_command(const std::string& line) {
    auto sp = line.find(' ');
    std::string cmd = (sp == std::string::npos) ? line : line.substr(0, sp);
    std::string rest = (sp == std::string::npos) ? "" : line.substr(sp + 1);

    if (cmd == "SET") {
        auto sp2 = rest.find(' ');
        if (sp2 == std::string::npos) return "ERR\n";
        store_.set(rest.substr(0, sp2), rest.substr(sp2 + 1));
        return "OK\n";
    }
    if (cmd == "GET") {
        auto val = store_.get(rest);
        return val ? "VALUE " + *val + "\n" : "NIL\n";
    }
    if (cmd == "DEL") {
        return store_.del(rest) ? "OK\n" : "NOT_FOUND\n";
    }
    if (cmd == "SCAN_PREFIX") {
        auto results = store_.scan(filter::key_prefix(rest));
        std::string response;
        for (const auto& [k, v] : results) {
            response += "RESULT " + k + " " + v + "\n";
        }
        return response + "END\n";
    }
    return "ERR\n";
}

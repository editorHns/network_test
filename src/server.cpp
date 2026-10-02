#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {
volatile std::sig_atomic_t running = 1;

void stop_server(int) {
    running = 0;
}

std::string url_decode(const std::string& value) {
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            char hex[3] = {value[i + 1], value[i + 2], '\0'};
            char* end = nullptr;
            const long code = std::strtol(hex, &end, 16);
            if (end && *end == '\0') {
                result += static_cast<char>(code);
                i += 2;
                continue;
            }
        }
        result += value[i] == '+' ? ' ' : value[i];
    }
    return result;
}

bool get_query_value(const std::string& query, const std::string& key, int& value) {
    const std::string prefix = key + "=";
    std::size_t pos = 0;

    while (pos < query.size()) {
        const std::size_t amp = query.find('&', pos);
        const std::string item = query.substr(
            pos, amp == std::string::npos ? std::string::npos : amp - pos);

        if (item.rfind(prefix, 0) == 0) {
            try {
                value = std::stoi(url_decode(item.substr(prefix.size())));
                return true;
            } catch (...) {
                return false;
            }
        }

        if (amp == std::string::npos) break;
        pos = amp + 1;
    }

    return false;
}

std::string make_response(int status, const std::string& body) {
    const std::string status_text = status == 200 ? "OK" : "Bad Request";
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << status_text << "\r\n"
             << "Content-Type: text/plain; charset=utf-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;
    return response.str();
}
}

int main(int argc, char* argv[]) {
    const int port = argc > 1 ? std::stoi(argv[1]) : 8080;

    std::signal(SIGINT, stop_server);
    std::signal(SIGTERM, stop_server);

    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket(): " << std::strerror(errno) << '\n';
        return 1;
    }

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "bind(): " << std::strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        std::cerr << "listen(): " << std::strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    std::cout << "Network Test server listening on http://127.0.0.1:" << port << '\n';
    std::cout << "Endpoint: GET /api/calc?a=10&b=20\n";

    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(
            server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_fd < 0) {
            if (!running) break;
            continue;
        }

        char buffer[4096]{};
        const ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        std::string response;
        if (received <= 0) {
            response = make_response(400, "Empty request\n");
        } else {
            const std::string request(buffer, static_cast<std::size_t>(received));
            std::istringstream request_stream(request);

            std::string method;
            std::string target;
            std::string version;
            request_stream >> method >> target >> version;

            if (method != "GET") {
                response = make_response(400, "Only GET is supported\n");
            } else {
                const std::size_t query_pos = target.find('?');
                const std::string path = target.substr(0, query_pos);
                const std::string query =
                    query_pos == std::string::npos ? "" : target.substr(query_pos + 1);

                if (path == "/health") {
                    response = make_response(200, "OK\n");
                } else if (path == "/api/calc") {
                    int a = 0;
                    int b = 0;
                    if (!get_query_value(query, "a", a) || !get_query_value(query, "b", b)) {
                        response = make_response(400, "Use /api/calc?a=<int>&b=<int>\n");
                    } else {
                        response = make_response(
                            200, std::to_string(a + b) + "\n");
                    }
                } else {
                    response = make_response(400, "Unknown endpoint\n");
                }
            }
        }

        send(client_fd, response.data(), response.size(), 0);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

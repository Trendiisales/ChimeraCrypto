#pragma once

#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <functional>

namespace chimera {

class SimpleHttpServer {
public:
    using StateCallback   = std::function<std::string()>;
    using CommandCallback = std::function<std::string(const std::string& cmd, const std::string& body)>;

    SimpleHttpServer(int port = 8080)
        : port_(port), running_(false), state_callback_(nullptr), command_callback_(nullptr) {
        const char* home = std::getenv("HOME");
        gui_dir_ = std::string(home ? home : "/home/jo") + "/ChimeraCrypto/gui/";
    }

    ~SimpleHttpServer() {
        stop();
    }

    void set_state_callback(StateCallback callback) {
        state_callback_ = callback;
    }

    // Wire this to handle POST /api/kill and POST /api/flatten
    // cmd = "kill_all" | "flatten:<sym>"
    // Returns JSON string to send back to client
    void set_command_callback(CommandCallback callback) {
        command_callback_ = callback;
    }
    
    bool start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "[HTTP] Failed to create socket\n";
            return false;
        }
        
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        // 1s accept timeout so stop() wakes the thread without hanging
        struct timeval tv{ 1, 0 };
        setsockopt(server_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "[HTTP] Failed to bind to port " << port_ << "\n";
            close(server_fd_);
            return false;
        }
        
        if (listen(server_fd_, 10) < 0) {
            std::cerr << "[HTTP] Failed to listen\n";
            close(server_fd_);
            return false;
        }
        
        running_ = true;
        server_thread_ = std::thread(&SimpleHttpServer::run, this);
        
        std::cout << "[HTTP] Server started on port " << port_ << "\n";
        std::cout << "[HTTP] GUI available at http://localhost:" << port_ << "\n";
        
        return true;
    }
    
    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
    
private:
    void run() {
        while (running_) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                // EAGAIN/EWOULDBLOCK = timeout, loop and check running_ again
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
                if (running_) std::cerr << "[HTTP] Accept error: " << strerror(errno) << "\n";
                continue;
            }
            
            handle_client(client_fd);
            close(client_fd);
        }
    }
    
    void handle_client(int client_fd) {
        char buffer[8192];
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) return;

        buffer[bytes] = '\0';
        std::string request(buffer);

        // Parse request line
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;

        // CORS preflight
        if (method == "OPTIONS") {
            std::string r =
                "HTTP/1.1 204 No Content\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n"
                "Connection: close\r\n\r\n";
            send_all(client_fd, r);
            return;
        }

        // Extract POST body (after blank line separator)
        std::string post_body;
        if (method == "POST") {
            auto sep = request.find("\r\n\r\n");
            if (sep != std::string::npos)
                post_body = request.substr(sep + 4);
        }

        if (path == "/" || path == "/index.html") {
            serve_file(client_fd, "index.html", "text/html");
        } else if (path == "/app.js") {
            serve_file(client_fd, "app.js", "application/javascript");
        } else if (path == "/style.css") {
            serve_file(client_fd, "style.css", "text/css");
        } else if (path == "/favicon.svg") {
            serve_file(client_fd, "favicon.svg", "image/svg+xml");
        } else if (path == "/api/state") {
            serve_state(client_fd);
        } else if (method == "POST" && path == "/api/kill") {
            // Emergency kill all — flatten every open position immediately
            serve_command(client_fd, "kill_all", post_body);
        } else if (method == "POST" && path == "/api/flatten") {
            // Flatten specific symbol: body = {"sym":"BTC"}
            serve_command(client_fd, "flatten", post_body);
        } else {
            send_404(client_fd);
        }
    }
    
    // send_all — guaranteed full send loop, handles partial sends on large responses
    static void send_all(int fd, const std::string& data) {
        const char* ptr = data.c_str();
        size_t remaining = data.size();
        while (remaining > 0) {
            ssize_t sent = ::send(fd, ptr, remaining, MSG_NOSIGNAL);
            if (sent <= 0) break;
            ptr += sent;
            remaining -= sent;
        }
    }

    void serve_file(int client_fd, const std::string& filename, const std::string& content_type) {
        std::string filepath = gui_dir_ + filename;
        std::ifstream file(filepath);
        
        if (!file) {
            send_404(client_fd);
            return;
        }
        
        std::ostringstream content;
        content << file.rdbuf();
        std::string body = content.str();
        
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: " << content_type << "\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Cache-Control: no-store, no-cache\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        
        send_all(client_fd, response.str());
    }
    
    void serve_command(int client_fd, const std::string& cmd, const std::string& body) {
        std::string result = command_callback_
            ? command_callback_(cmd, body)
            : "{\"ok\":false,\"error\":\"no handler\"}";

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: application/json\r\n"
                 << "Content-Length: " << result.size() << "\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Connection: close\r\n\r\n"
                 << result;
        send_all(client_fd, response.str());
    }

    void serve_state(int client_fd) {
        std::string json = state_callback_ ? state_callback_() : "{}";
        
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json.size() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Cache-Control: no-store, no-cache\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << json;
        
        send_all(client_fd, response.str());
    }
    
    void send_404(int client_fd) {
        std::string response = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Not Found";
        send(client_fd, response.c_str(), response.size(), 0);
    }
    
    int port_;
    int server_fd_ = -1;
    std::atomic<bool> running_;
    std::thread server_thread_;
    StateCallback   state_callback_;
    CommandCallback command_callback_;
    std::string gui_dir_;  // resolved at construction: $HOME/ChimeraCrypto/gui/
};

} // namespace chimera

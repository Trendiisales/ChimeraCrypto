#pragma once

#include <string>
#include <sstream>
#include <thread>
#include <atomic>
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
    using StateCallback = std::function<std::string()>;
    
    SimpleHttpServer(int port = 8080) 
        : port_(port), running_(false), state_callback_(nullptr) {}
    
    ~SimpleHttpServer() {
        stop();
    }
    
    void set_state_callback(StateCallback callback) {
        state_callback_ = callback;
    }
    
    bool start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "[HTTP] Failed to create socket\n";
            return false;
        }
        
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
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
                if (running_) {
                    std::cerr << "[HTTP] Accept failed\n";
                }
                continue;
            }
            
            handle_client(client_fd);
            close(client_fd);
        }
    }
    
    void handle_client(int client_fd) {
        char buffer[4096];
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) return;
        
        buffer[bytes] = '\0';
        std::string request(buffer);
        
        // Parse request line
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;
        
        if (path == "/" || path == "/index.html") {
            serve_file(client_fd, "index.html", "text/html");
        } else if (path == "/app.js") {
            serve_file(client_fd, "app.js", "application/javascript");
        } else if (path == "/style.css") {
            serve_file(client_fd, "style.css", "text/css");
        } else if (path == "/api/state") {
            serve_state(client_fd);
        } else {
            send_404(client_fd);
        }
    }
    
    void serve_file(int client_fd, const std::string& filename, const std::string& content_type) {
        std::string filepath = "/home/jo/ChimeraCrypto/gui/" + filename;
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
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        
        std::string resp_str = response.str();
        send(client_fd, resp_str.c_str(), resp_str.size(), 0);
    }
    
    void serve_state(int client_fd) {
        std::string json = state_callback_ ? state_callback_() : "{}";
        
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json.size() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << json;
        
        std::string resp_str = response.str();
        send(client_fd, resp_str.c_str(), resp_str.size(), 0);
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
    StateCallback state_callback_;
};

} // namespace chimera

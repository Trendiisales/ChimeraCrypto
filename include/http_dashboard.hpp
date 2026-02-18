#pragma once
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include "TelemetryState.hpp"
#include "TelemetryJson.hpp"
#include "ExecutionSimulator.hpp"
#include "DepthManager.hpp"

namespace chimera {

class HttpDashboard {
public:
    HttpDashboard() = default;
    
    ~HttpDashboard() {
        stop();
    }
    
    void set_telemetry(TelemetryState* telem) {
        telemetry_ = telem;
    }
    
    void set_execution_sim(void* exec_sim) {
        exec_sim_ = exec_sim;
    }
    
    void set_depth_managers(void* btc_depth, void* eth_depth) {
        btc_depth_ = btc_depth;
        eth_depth_ = eth_depth;
    }
    
    bool start(int port = 8888) {
        port_ = port;
        
        // Load dashboard HTML
        if (!load_dashboard_html()) {
            dashboard_html_ = "<html><body><h1>Chimera</h1><p>Dashboard not found</p></body></html>";
        }
        
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) return false;
        
        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);
        
        if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(server_socket_);
            return false;
        }
        
        if (listen(server_socket_, 10) < 0) {
            close(server_socket_);
            return false;
        }
        
        running_.store(true, std::memory_order_release);
        server_thread_ = std::thread(&HttpDashboard::server_loop, this);
        
        return true;
    }
    
    void stop() {
        running_.store(false, std::memory_order_release);
        if (server_socket_ >= 0) {
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
            server_socket_ = -1;
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

private:
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int server_socket_{-1};
    int port_{8888};
    std::string dashboard_html_;
    
    TelemetryState* telemetry_{nullptr};
    void* exec_sim_{nullptr};
    void* btc_depth_{nullptr};
    void* eth_depth_{nullptr};
    
    bool load_dashboard_html() {
        std::ifstream file("dashboard.html");
        if (!file.is_open()) {
            file.open("../dashboard.html");
            if (!file.is_open()) {
                return false;
            }
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        dashboard_html_ = buffer.str();
        return true;
    }
    
    std::string generate_json_api() const {
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        
        double total_pnl = 0.0;
        double equity = 10000.0;
        int fills = 0;
        int wins = 0;
        int losses = 0;
        double avg_slip = 0.0;
        double worst_slip = 0.0;
        
        if (exec_sim_) {
            auto* sim = reinterpret_cast<ExecutionSimulator*>(exec_sim_);
            total_pnl = sim->realized_pnl();
            equity = sim->equity();
            fills = sim->fills();
            wins = sim->wins();
            losses = sim->losses();
            avg_slip = sim->avg_slippage();
            worst_slip = sim->worst_slippage();
            
            // Debug: print once per 10 seconds
            static uint64_t last_debug = 0;
            uint64_t now = std::time(nullptr);
            if (now - last_debug > 10) {
                printf("[DASHBOARD_DEBUG] exec_sim: pnl=%.2f equity=%.2f fills=%d\n", 
                       total_pnl, equity, fills);
                last_debug = now;
            }
        }
        
        double btc_price = 0.0;
        double eth_price = 0.0;
        double btc_imb = 0.0;
        double eth_imb = 0.0;
        
        if (btc_depth_) {
            auto* btc_dm = reinterpret_cast<DepthManager*>(btc_depth_);
            auto btc_view = btc_dm->book();
            btc_price = btc_view.mid;
            btc_imb = btc_view.imbalance;
            
            // Debug
            static uint64_t last_price_debug = 0;
            uint64_t now = std::time(nullptr);
            if (now - last_price_debug > 5) {
                printf("[PRICE_DEBUG] BTC: book().mid=%.2f book().ready=%s\n", 
                       btc_price, btc_view.ready ? "YES" : "NO");
                last_price_debug = now;
            }
        }
        
        if (eth_depth_) {
            auto* eth_dm = reinterpret_cast<DepthManager*>(eth_depth_);
            auto eth_view = eth_dm->book();
            eth_price = eth_view.mid;
            eth_imb = eth_view.imbalance;
        }
        
        // Sanitize all floats - NaN breaks JSON parsing
        auto safe = [](double v) { return std::isfinite(v) ? v : 0.0; };
        total_pnl = safe(total_pnl);
        equity = safe(equity);
        avg_slip = safe(avg_slip);
        worst_slip = safe(worst_slip);
        btc_price = safe(btc_price);
        eth_price = safe(eth_price);
        btc_imb = safe(btc_imb);
        eth_imb = safe(eth_imb);
        
        // Get exposure and quality from telemetry if available
        double exposure = 0.0;
        double quality = 100.0;
        if (telemetry_) {
            auto* telem = reinterpret_cast<TelemetryState*>(telemetry_);
            exposure = telem->risk.exposure;
            quality = (fills > 0) ? ((double)wins / fills * 100.0) : 100.0;
        }
        exposure = safe(exposure);
        quality = safe(quality);
        
        json << "{"
             << "\"timestamp\":" << std::time(nullptr) << ","
             << "\"total_pnl\":" << total_pnl << ","
             << "\"equity\":" << equity << ","
             << "\"fills\":" << fills << ","
             << "\"wins\":" << wins << ","
             << "\"losses\":" << losses << ","
             << "\"avg_slippage\":" << avg_slip << ","
             << "\"worst_slippage\":" << worst_slip << ","
             << "\"exposure\":" << exposure << ","
             << "\"quality\":" << quality << ","
             << "\"btc_price\":" << btc_price << ","
             << "\"eth_price\":" << eth_price << ","
             << "\"btc_imbalance\":" << btc_imb << ","
             << "\"eth_imbalance\":" << eth_imb
             << "}";
        
        std::string result = json.str();
        
        // Debug: print actual JSON once per 10 seconds
        static uint64_t last_json_debug = 0;
        uint64_t now2 = std::time(nullptr);
        if (now2 - last_json_debug > 10) {
            printf("[JSON_DEBUG] %s\n", result.c_str());
            last_json_debug = now2;
        }
        
        return result;
    }
    
    void server_loop() {
        while (running_.load(std::memory_order_acquire)) {
            struct sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) continue;
            
            char buffer[4096] = {0};
            read(client_socket, buffer, sizeof(buffer) - 1);
            
            std::string request(buffer);
            std::string content;
            std::string content_type = "text/html";
            
            if (request.find("GET /api") == 0) {
                content = generate_json_api();
                content_type = "application/json";
            } else {
                content = dashboard_html_;
            }
            
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: " << content_type << "\r\n"
                     << "Content-Length: " << content.size() << "\r\n"
                     << "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                     << "Pragma: no-cache\r\n"
                     << "Expires: 0\r\n"
                     << "Access-Control-Allow-Origin: *\r\n"
                     << "Connection: close\r\n\r\n"
                     << content;
            
            std::string resp_str = response.str();
            send(client_socket, resp_str.c_str(), resp_str.size(), 0);
            close(client_socket);
        }
    }
};

} // namespace chimera

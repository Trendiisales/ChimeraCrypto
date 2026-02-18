#pragma once
#include "TelemetryState.hpp"
#include "TelemetryJson.hpp"
#include <libwebsockets.h>
#include <thread>
#include <atomic>
#include <cstring>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// TELEMETRY WEBSOCKET SERVER
// Broadcasts telemetry JSON to connected dashboard clients
// ═══════════════════════════════════════════════════════════════════

class TelemetryServer {
private:
    static TelemetryState* g_state_;
    static std::atomic<bool> g_running_;
    
    struct lws_context* context_{nullptr};
    std::thread server_thread_;
    
    static int callback_ws(struct lws* wsi,
                          enum lws_callback_reasons reason,
                          void* user,
                          void* in,
                          size_t len) {
        switch (reason) {
            case LWS_CALLBACK_ESTABLISHED:
                lwsl_notice("Telemetry client connected\n");
                lws_callback_on_writable(wsi);
                break;
                
            case LWS_CALLBACK_SERVER_WRITEABLE: {
                if (!g_state_) break;
                
                std::string json = TelemetryJson::build(*g_state_);
                
                // Allocate buffer with LWS_PRE padding
                size_t json_len = json.size();
                unsigned char* buffer = new unsigned char[LWS_PRE + json_len];
                unsigned char* p = &buffer[LWS_PRE];
                
                std::memcpy(p, json.c_str(), json_len);
                
                lws_write(wsi, p, json_len, LWS_WRITE_TEXT);
                
                delete[] buffer;
                
                // Schedule next write
                lws_callback_on_writable(wsi);
                break;
            }
            
            case LWS_CALLBACK_CLOSED:
                lwsl_notice("Telemetry client disconnected\n");
                break;
                
            default:
                break;
        }
        
        return 0;
    }
    
    void server_loop() {
        static struct lws_protocols protocols[] = {
            {
                "telemetry-protocol",
                callback_ws,
                0,
                8192,
                0,
                nullptr,
                0
            },
            { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
        };
        
        struct lws_context_creation_info info{};
        info.port = 9000;
        info.protocols = protocols;
        info.gid = -1;
        info.uid = -1;
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        
        context_ = lws_create_context(&info);
        if (!context_) {
            lwsl_err("Failed to create telemetry WebSocket context\n");
            return;
        }
        
        lwsl_notice("Telemetry WebSocket server started on port 9000\n");
        
        while (g_running_.load(std::memory_order_acquire)) {
            lws_service(context_, 250);  // 250ms update rate
        }
        
        lws_context_destroy(context_);
        context_ = nullptr;
    }
    
public:
    TelemetryServer() = default;
    
    ~TelemetryServer() {
        stop();
    }
    
    bool start(TelemetryState* state) {
        if (!state) return false;
        
        g_state_ = state;
        g_running_.store(true, std::memory_order_release);
        
        server_thread_ = std::thread(&TelemetryServer::server_loop, this);
        
        return true;
    }
    
    void stop() {
        g_running_.store(false, std::memory_order_release);
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
};

// Static member definitions
TelemetryState* TelemetryServer::g_state_ = nullptr;
std::atomic<bool> TelemetryServer::g_running_{false};

} // namespace chimera

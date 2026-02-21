#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <libwebsockets.h>
#include "telemetry/TelemetrySpine.hpp"

namespace chimera {

class WsTelemetryServer {
public:
    WsTelemetryServer(int port,
                      TelemetrySpine& spine,
                      const std::string& static_dir);

    ~WsTelemetryServer();

    void start();
    void stop();
    
    // NEW: Broadcast arbitrary JSON message to all connected clients
    void broadcast(const std::string& json_message);

    // MUST BE PUBLIC for libwebsockets protocol table
    static int callback_ws(struct lws* wsi,
                           enum lws_callback_reasons reason,
                           void* user,
                           void* in,
                           size_t len);

private:
    int port_;
    TelemetrySpine& spine_;
    std::string static_dir_;

    struct lws_context* context_;
    std::thread service_thread_;
    std::atomic<bool> running_;
    
    // NEW: Message queue for broadcasting
    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;

    static WsTelemetryServer* self_;
};

}

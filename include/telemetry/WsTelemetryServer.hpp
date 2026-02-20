#pragma once

#include <string>
#include <thread>
#include <atomic>
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

    static WsTelemetryServer* self_;
};

}

#include "telemetry/WsTelemetryServer.hpp"
#include <cstring>
#include <iostream>

using namespace chimera;

WsTelemetryServer* WsTelemetryServer::self_ = nullptr;

WsTelemetryServer::WsTelemetryServer(int port,
                                     TelemetrySpine& spine,
                                     const std::string& static_dir)
    : port_(port),
      spine_(spine),
      static_dir_(static_dir),
      context_(nullptr),
      running_(false)
{
    self_ = this;
}

WsTelemetryServer::~WsTelemetryServer()
{
    stop();
}

int WsTelemetryServer::callback_ws(struct lws* wsi,
                                    enum lws_callback_reasons reason,
                                    void* user,
                                    void* in,
                                    size_t len)
{
    switch (reason) {

    case LWS_CALLBACK_ESTABLISHED:
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        if (!self_) break;

        std::string json = self_->spine_.json();

        unsigned char buffer[LWS_PRE + 4096];
        unsigned char* p = &buffer[LWS_PRE];

        size_t msg_len = json.size();
        if (msg_len > 4096)
            msg_len = 4096;

        std::memcpy(p, json.c_str(), msg_len);

        lws_write(wsi, p, msg_len, LWS_WRITE_TEXT);

        lws_callback_on_writable(wsi);
        break;
    }

    default:
        break;
    }

    return 0;
}

void WsTelemetryServer::start()
{
    struct lws_protocols protocols[] = {
        { "ws", WsTelemetryServer::callback_ws, 0, 4096 },
        { nullptr, nullptr, 0, 0 }
    };

    struct lws_context_creation_info info;
    std::memset(&info, 0, sizeof(info));

    info.port = port_;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context_ = lws_create_context(&info);

    if (!context_)
        throw std::runtime_error("Failed to create LWS context");

    running_ = true;

    service_thread_ = std::thread([this]() {
        while (running_) {
            lws_service(context_, 50);
        }
    });

    std::cout << "WS telemetry listening on port " << port_ << std::endl;
}

void WsTelemetryServer::stop()
{
    if (!running_)
        return;

    running_ = false;

    if (service_thread_.joinable())
        service_thread_.join();

    if (context_) {
        lws_context_destroy(context_);
        context_ = nullptr;
    }
}

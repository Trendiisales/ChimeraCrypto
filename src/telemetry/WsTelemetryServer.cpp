#include "telemetry/WsTelemetryServer.hpp"
#include <cstring>
#include <iostream>
#include <queue>
#include <mutex>
#include <set>

using namespace chimera;

WsTelemetryServer* WsTelemetryServer::self_ = nullptr;
static std::set<struct lws*> connected_clients;
static std::mutex clients_mutex;

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

void WsTelemetryServer::broadcast(const std::string& json_message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.push(json_message);
    
    if (context_) {
        std::lock_guard<std::mutex> client_lock(clients_mutex);
        for (auto* wsi : connected_clients) {
            lws_callback_on_writable(wsi);
        }
    }
}

int WsTelemetryServer::callback_ws(struct lws* wsi,
                                    enum lws_callback_reasons reason,
                                    void* user,
                                    void* in,
                                    size_t len)
{
    switch (reason) {

    case LWS_CALLBACK_ESTABLISHED:
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        connected_clients.insert(wsi);
        std::printf("[WS] Client connected, total: %zu\n", connected_clients.size());
        std::fflush(stdout);
        lws_callback_on_writable(wsi);
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        if (!self_) break;

        std::string msg_to_send;
        {
            std::lock_guard<std::mutex> lock(self_->queue_mutex_);
            if (!self_->message_queue_.empty()) {
                msg_to_send = self_->message_queue_.front();
                self_->message_queue_.pop();
            } else {
                msg_to_send = self_->spine_.json();
            }
        }

        unsigned char buffer[LWS_PRE + 4096];
        unsigned char* p = &buffer[LWS_PRE];

        size_t msg_len = msg_to_send.size();
        if (msg_len > 4096)
            msg_len = 4096;

        std::memcpy(p, msg_to_send.c_str(), msg_len);

        lws_write(wsi, p, msg_len, LWS_WRITE_TEXT);

        {
            std::lock_guard<std::mutex> lock(self_->queue_mutex_);
            if (!self_->message_queue_.empty()) {
                lws_callback_on_writable(wsi);
            }
        }
        break;
    }

    case LWS_CALLBACK_CLOSED:
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        connected_clients.erase(wsi);
        std::printf("[WS] Client disconnected, remaining: %zu\n", connected_clients.size());
        std::fflush(stdout);
        break;
    }

    case LWS_CALLBACK_CLOSED:
        std::printf("[WS] Client disconnected\n");
        std::fflush(stdout);
        break;

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
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        connected_clients.clear();
    }
}

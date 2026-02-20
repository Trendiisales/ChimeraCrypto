#pragma once

#include <thread>
#include <atomic>
#include <string>

namespace chimera {

class InstitutionalEngine;

class TelemetryServer {
public:
    TelemetryServer(InstitutionalEngine& engine,
                    int port = 8080);

    ~TelemetryServer();

    void start();
    void stop();

private:
    void run();
    std::string build_json();

    InstitutionalEngine& engine_;
    int port_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
};

}

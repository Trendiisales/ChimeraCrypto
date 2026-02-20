#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include "core/SpscRing.hpp"

namespace chimera {

class InstitutionalEngine;

class EngineRuntime {
public:
    explicit EngineRuntime(InstitutionalEngine& engine);
    ~EngineRuntime();

    void start();
    void stop();

    void post_market_event(const std::string& symbol,
                           std::function<void()> fn);

    void post_execution_event(const std::string& symbol,
                              std::function<void()> fn);

private:
    void market_loop();
    void execution_loop();
    void pin_thread_to_core(std::thread& t, int core_id);

    InstitutionalEngine& engine_;

    std::thread market_thread_;
    std::thread execution_thread_;

    std::atomic<bool> running_{false};

    SpscRing<std::function<void()>, 1024> market_ring_;
    SpscRing<std::function<void()>, 1024> exec_ring_;
};

}

#pragma once
#include <atomic>
#include "telemetry/DeskSnapshot.hpp"

namespace chimera {

class TelemetrySpine {
public:
    TelemetrySpine() = default;

    void publish(DeskSnapshot* snap) {
        snapshot_.store(snap, std::memory_order_release);
    }

    const char* json() const {
        DeskSnapshot* s = snapshot_.load(std::memory_order_acquire);
        static DeskSnapshot fallback;
        if (!s) s = &fallback;
        return s->to_json();
    }

private:
    std::atomic<DeskSnapshot*> snapshot_{nullptr};
};

}

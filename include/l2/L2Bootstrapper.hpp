#pragma once
#include <functional>
#include <string>
#include <chrono>
#include "L2Book.hpp"
#include "risk/RiskGovernor.hpp"
#include "spine/EventSpine.hpp"

namespace chimera {

class L2Bootstrapper {
public:
    L2Bootstrapper(const std::string& symbol,
                   RiskGovernor& governor,
                   EventSpine& spine);

    void set_snapshot_request_callback(
        std::function<void(const std::string&)> cb);

    void on_snapshot(const Snapshot& snap);
    void on_ws_delta(const DepthEvent& ev);

    const L2Book& book() const;
    bool ready() const;

private:
    std::string symbol_;
    L2Book book_;
    uint64_t last_update_id_ = 0;
    bool bootstrapped_ = false;

    int resync_count_ = 0;
    std::chrono::steady_clock::time_point last_resync_;

    std::function<void(const std::string&)> snapshot_cb_;

    RiskGovernor& governor_;
    EventSpine& spine_;

    void register_resync();
};

}

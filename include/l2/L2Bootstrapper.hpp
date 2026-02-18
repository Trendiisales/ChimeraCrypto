#pragma once
#include <deque>
#include <functional>
#include <string>
#include "L2Types.hpp"
#include "L2Book.hpp"

namespace chimera {

class L2Bootstrapper {
public:
    using SnapshotRequestCallback = std::function<void(const std::string&)>;

    L2Bootstrapper(const std::string& symbol);

    void set_snapshot_request_callback(SnapshotRequestCallback cb);

    void on_ws_delta(const DepthEvent& ev);
    void on_snapshot(const Snapshot& snap);

    bool ready() const;
    const L2Book& book() const;

    void force_resync();

private:
    void try_apply_buffer();

    std::string symbol_;
    std::deque<DepthEvent> buffer_;
    L2Book book_;

    bool snapshot_loaded_;
    bool snapshot_requested_;
    SnapshotRequestCallback snapshot_request_cb_;
};

}

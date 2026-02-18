#include "l2/L2Bootstrapper.hpp"
#include <cstdio>

namespace chimera {

enum class L2State {
    WAITING_SNAPSHOT,
    REPLAYING,
    LIVE
};

static L2State g_state = L2State::WAITING_SNAPSHOT;

L2Bootstrapper::L2Bootstrapper(const std::string& symbol)
    : symbol_(symbol),
      snapshot_loaded_(false),
      snapshot_requested_(false)
{
    g_state = L2State::WAITING_SNAPSHOT;
}

void L2Bootstrapper::set_snapshot_request_callback(SnapshotRequestCallback cb) {
    snapshot_request_cb_ = cb;
}

void L2Bootstrapper::on_ws_delta(const DepthEvent& ev) {
    
    // LIVE mode - apply with RELAXED validation (monotonic only)
    if (g_state == L2State::LIVE) {
        if (!book_.apply_delta(ev.U, ev.u, ev.bids, ev.asks, L2ValidationMode::RELAXED)) {
            printf("[L2_BOOT] %s: Corruption in LIVE mode → forcing resync\n", symbol_.c_str());
            force_resync();
        }
        return;
    }

    // WAITING_SNAPSHOT or REPLAYING - buffer everything
    buffer_.push_back(ev);

    if (buffer_.size() > 1000) {
        buffer_.pop_front();
    }

    // Request snapshot if not already requested
    if (g_state == L2State::WAITING_SNAPSHOT && !snapshot_requested_) {
        snapshot_requested_ = true;
        if (snapshot_request_cb_) {
            printf("[L2_BOOT] %s: Requesting snapshot\n", symbol_.c_str());
            snapshot_request_cb_(symbol_);
        }
    }

    // If in REPLAYING, try to apply buffer
    if (g_state == L2State::REPLAYING) {
        try_apply_buffer();
    }
}

void L2Bootstrapper::on_snapshot(const Snapshot& snap) {
    printf("[L2_BOOT] %s: Snapshot received, lastUpdateId=%lu, bids=%zu, asks=%zu\n",
           symbol_.c_str(), snap.lastUpdateId, snap.bids.size(), snap.asks.size());

    book_.load_snapshot(snap.lastUpdateId, snap.bids, snap.asks);

    snapshot_loaded_ = true;
    snapshot_requested_ = false;
    g_state = L2State::REPLAYING;

    // Drop all buffered deltas where u <= lastUpdateId
    size_t original_size = buffer_.size();
    while (!buffer_.empty() && buffer_.front().u <= snap.lastUpdateId) {
        buffer_.pop_front();
    }

    printf("[L2_BOOT] %s: After snapshot, buffer size=%zu (dropped %zu old events)\n",
           symbol_.c_str(), buffer_.size(), original_size - buffer_.size());

    // Try to apply remaining buffer with STRICT validation
    try_apply_buffer();
}

void L2Bootstrapper::try_apply_buffer() {
    
    while (!buffer_.empty()) {
        const auto& ev = buffer_.front();

        // Use STRICT validation during replay
        if (!book_.apply_delta(ev.U, ev.u, ev.bids, ev.asks, L2ValidationMode::STRICT)) {
            printf("[L2_BOOT] %s: STRICT gap during replay → forcing resync\n", symbol_.c_str());
            force_resync();
            return;
        }

        buffer_.pop_front();
    }

    // Buffer fully replayed - switch to LIVE
    g_state = L2State::LIVE;
    printf("[L2_BOOT] %s: ✓ LIVE at updateId=%lu\n", symbol_.c_str(), book_.last_update_id());
}

bool L2Bootstrapper::ready() const {
    return g_state == L2State::LIVE && book_.is_synced();
}

const L2Book& L2Bootstrapper::book() const {
    return book_;
}

void L2Bootstrapper::force_resync() {
    printf("[L2_BOOT] %s: Forcing resync\n", symbol_.c_str());

    book_.clear();
    buffer_.clear();

    snapshot_loaded_ = false;
    snapshot_requested_ = true;
    g_state = L2State::WAITING_SNAPSHOT;

    if (snapshot_request_cb_) {
        snapshot_request_cb_(symbol_);
    }
}

} // namespace chimera

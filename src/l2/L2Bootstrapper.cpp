#include "l2/L2Bootstrapper.hpp"
#include "spine/Event.hpp"

namespace chimera {

L2Bootstrapper::L2Bootstrapper(const std::string& symbol,
                               RiskGovernor& governor,
                               EventSpine& spine)
    : symbol_(symbol),
      governor_(governor),
      spine_(spine)
{}

void L2Bootstrapper::set_snapshot_request_callback(
    std::function<void(const std::string&)> cb) {
    snapshot_cb_ = std::move(cb);
}

void L2Bootstrapper::on_snapshot(const Snapshot& snap) {
    last_update_id_ = snap.last_update_id;
    book_.load_snapshot(snap, last_update_id_);
    bootstrapped_ = true;
}

void L2Bootstrapper::on_ws_delta(const DepthEvent& ev) {
    if (!bootstrapped_) {
        if (snapshot_cb_)
            snapshot_cb_(symbol_);
        return;
    }

    bool ok = book_.apply_delta(ev);

    if (!ok || book_.needs_resync()) {
        register_resync();
        bootstrapped_ = false;

        if (snapshot_cb_)
            snapshot_cb_(symbol_);
    }
}

void L2Bootstrapper::register_resync() {
    auto now = std::chrono::steady_clock::now();

    if (resync_count_ == 0)
        last_resync_ = now;

    resync_count_++;

    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - last_resync_).count();

    if (elapsed <= 60 && resync_count_ >= 3) {
        governor_.force_halt("DEPTH_DESYNC");

        Event ev;
        ev.type = EventType::SYSTEM_ALERT;
        ev.payload = SystemEvent{ "DEPTH_DESYNC on " + symbol_ };
        ev.sequence = 0;
        spine_.publish(ev);
    }

    if (elapsed > 60) {
        resync_count_ = 1;
        last_resync_ = now;
    }
}

const L2Book& L2Bootstrapper::book() const {
    return book_;
}

bool L2Bootstrapper::ready() const {
    return bootstrapped_ && book_.ready();
}

}

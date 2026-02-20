#include "l2/L2Book.hpp"
#include <algorithm>

namespace chimera {

void L2Book::load_snapshot(const Snapshot& snap,
                           uint64_t last_update_id) {
    bids_ = snap.bids;
    asks_ = snap.asks;
    last_update_id_ = last_update_id;
    ready_ = true;
    resync_required_ = false;
}

bool L2Book::apply_delta(const DepthEvent& ev) {
    if (!ready_)
        return false;

    uint64_t U = ev.first_update_id;
    uint64_t u = ev.final_update_id;

    if (U > last_update_id_ + 1) {
        resync_required_ = true;
        ready_ = false;
        return false;
    }

    if (u <= last_update_id_)
        return true;

    apply_levels(bids_, ev.bids);
    apply_levels(asks_, ev.asks);

    last_update_id_ = u;
    return true;
}

void L2Book::apply_levels(std::vector<Level>& side,
                          const std::vector<Level>& updates) {
    for (const auto& lvl : updates) {
        auto it = std::find_if(
            side.begin(),
            side.end(),
            [&](const Level& l) { return l.price == lvl.price; });

        if (lvl.qty == 0) {
            if (it != side.end())
                side.erase(it);
        } else {
            if (it != side.end()) {
                it->qty = lvl.qty;
            } else {
                side.push_back(lvl);
            }
        }
    }

    std::sort(side.begin(), side.end(),
        [&](const Level& a, const Level& b) {
            return &side == &bids_
                ? a.price > b.price
                : a.price < b.price;
        });
}

const std::vector<Level>& L2Book::bids() const {
    return bids_;
}

const std::vector<Level>& L2Book::asks() const {
    return asks_;
}

bool L2Book::ready() const {
    return ready_;
}

bool L2Book::needs_resync() const {
    return resync_required_;
}

}

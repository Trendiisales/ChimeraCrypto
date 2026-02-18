#include "l2/L2Book.hpp"
#include <cstdio>

namespace chimera {

L2Book::L2Book() {
    clear();
}

void L2Book::clear() {
    bids_.clear();
    asks_.clear();
    last_update_id_ = 0;
    synced_ = false;
}

void L2Book::load_snapshot(uint64_t last_update_id,
                           const std::vector<L2Level>& bids,
                           const std::vector<L2Level>& asks) {
    clear();

    for (const auto& lvl : bids) {
        if (lvl.quantity > 0.0)
            bids_[lvl.price] = lvl.quantity;
    }

    for (const auto& lvl : asks) {
        if (lvl.quantity > 0.0)
            asks_[lvl.price] = lvl.quantity;
    }

    last_update_id_ = last_update_id;
    synced_ = true;
    
    printf("[L2] Snapshot applied lastUpdateId=%lu\n", last_update_id_);
}

bool L2Book::apply_delta(uint64_t U, uint64_t u,
                         const std::vector<L2Level>& bids,
                         const std::vector<L2Level>& asks,
                         L2ValidationMode mode) {
    
    if (!synced_)
        return false;

    // Rule 1: discard old events
    if (u <= last_update_id_)
        return true;

    // Rule 2: validation based on mode
    if (mode == L2ValidationMode::STRICT) {
        // Bootstrap/replay: enforce strict Binance overlap rule
        uint64_t expected = last_update_id_ + 1;
        
        if (!(U <= expected && u >= expected)) {
            printf("[L2] STRICT GAP: U=%lu u=%lu last=%lu expected=%lu\n",
                   U, u, last_update_id_, expected);
            synced_ = false;
            return false;
        }
    }
    // RELAXED mode: accept any monotonic update (u > lastUpdateId)
    // This handles Binance @depth@100ms batching correctly

    // Apply updates
    for (const auto& lvl : bids) {
        if (lvl.quantity == 0.0)
            bids_.erase(lvl.price);
        else
            bids_[lvl.price] = lvl.quantity;
    }

    for (const auto& lvl : asks) {
        if (lvl.quantity == 0.0)
            asks_.erase(lvl.price);
        else
            asks_[lvl.price] = lvl.quantity;
    }

    last_update_id_ = u;
    return true;
}

double L2Book::best_bid() const {
    if (bids_.empty())
        return 0.0;
    return bids_.begin()->first;
}

double L2Book::best_ask() const {
    if (asks_.empty())
        return 0.0;
    return asks_.begin()->first;
}

double L2Book::mid() const {
    double bid = best_bid();
    double ask = best_ask();
    if (bid == 0.0 || ask == 0.0)
        return 0.0;
    return (bid + ask) / 2.0;
}

double L2Book::imbalance_top5() const {
    if (bids_.empty() || asks_.empty())
        return 0.5;
    
    double bid_vol = 0.0;
    double ask_vol = 0.0;
    
    int count = 0;
    for (auto it = bids_.begin(); it != bids_.end() && count < 5; ++it, ++count)
        bid_vol += it->second;
    
    count = 0;
    for (auto it = asks_.begin(); it != asks_.end() && count < 5; ++it, ++count)
        ask_vol += it->second;
    
    if (bid_vol + ask_vol == 0.0)
        return 0.5;
    
    return bid_vol / (bid_vol + ask_vol);
}

} // namespace chimera

#pragma once
#include <cstdint>
#include <cmath>
#include <deque>
#include <algorithm>
#include "core/SymbolIndex.hpp"

namespace chimera {

// ============================================================================
// LeadLagEngine — N-symbol BTC lead-lag detector
//
// BTC (id=0) is always the leader.
// All other symbols (id=1..MAX_SYMBOLS-1) are potential followers.
//
// MEASURED LATENCY (Tokyo VPS → Binance AWS Tokyo):
//   WS feed p95: 18-25ms
//   BTC→follower propagation: 50-200ms
//   Remaining edge window: 25-175ms (conservative: 75ms)
//
// SIGNAL: BTC moves >= BTC_MOVE_THRESHOLD_BP in LOOKBACK_MS
//         AND follower has NOT yet moved TARGET_MOVED_MAX_BP
//
// ETH→SOL secondary: ETH leads SOL by ~30-80ms (separate check)
// ============================================================================

struct PricePoint {
    double  price;
    int64_t ts_ms;
};

class LeadLagEngine {
public:
    static constexpr double  MAX_LATENCY_MS        = 35.0;
    static constexpr double  BTC_MOVE_THRESHOLD_BP = 8.0;
    static constexpr double  TARGET_MOVED_MAX_BP   = 3.0;
    static constexpr int64_t LOOKBACK_MS           = 100;
    static constexpr int     MIN_BTC_SAMPLES       = 3;

    LeadLagEngine() {}

    void update_price(int symbol_id, double price, int64_t now_ms) {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return;
        auto& buf = buffers_[symbol_id];
        buf.push_back({price, now_ms});
        while (!buf.empty() && now_ms - buf.front().ts_ms > 500)
            buf.pop_front();
    }

    // BTC → target_id lead-lag signal
    // target_id must be != 0 (BTC doesn't follow itself)
    bool check_signal(int target_id, double latency_ms, int& direction) const {
        if (target_id == 0) return false;
        if (target_id < 0 || target_id >= MAX_SYMBOLS) return false;
        if (latency_ms > MAX_LATENCY_MS) return false;

        const auto& btc = buffers_[0];
        const auto& tgt = buffers_[target_id];

        if (btc.empty() || tgt.empty()) return false;
        if ((int)btc.size() < MIN_BTC_SAMPLES) return false;

        int64_t now_ms = btc.back().ts_ms;

        // BTC move in lookback window
        double btc_ref = 0.0;
        int btc_count = 0;
        for (auto it = btc.rbegin(); it != btc.rend(); ++it) {
            if (now_ms - it->ts_ms > LOOKBACK_MS) break;
            btc_ref = it->price;
            btc_count++;
        }
        if (btc_count < MIN_BTC_SAMPLES || btc_ref == 0.0) return false;

        double btc_now   = btc.back().price;
        double btc_delta = (btc_now - btc_ref) / btc_ref * 10000.0;
        if (std::fabs(btc_delta) < BTC_MOVE_THRESHOLD_BP) return false;

        // Target must NOT have moved much yet
        double tgt_ref = 0.0;
        for (auto it = tgt.rbegin(); it != tgt.rend(); ++it) {
            if (now_ms - it->ts_ms > LOOKBACK_MS) break;
            tgt_ref = it->price;
        }
        if (tgt_ref == 0.0) return false;

        double tgt_now   = tgt.back().price;
        double tgt_delta = (tgt_now - tgt_ref) / tgt_ref * 10000.0;

        bool same_dir = (btc_delta > 0) == (tgt_delta > 0);
        if (same_dir && std::fabs(tgt_delta) >= TARGET_MOVED_MAX_BP) return false;

        direction = (btc_delta > 0) ? 1 : -1;
        return true;
    }

    double btc_move_bp() const {
        const auto& btc = buffers_[0];
        if (btc.size() < 2) return 0.0;
        int64_t now_ms = btc.back().ts_ms;
        double ref = 0.0;
        for (auto it = btc.rbegin(); it != btc.rend(); ++it) {
            if (now_ms - it->ts_ms > LOOKBACK_MS) break;
            ref = it->price;
        }
        if (ref == 0.0) return 0.0;
        return (btc.back().price - ref) / ref * 10000.0;
    }

    // ETH (id=1) → SOL (id=2) secondary lead-lag
    bool check_signal_eth_sol(double latency_ms, int& direction) const {
        if (latency_ms > MAX_LATENCY_MS) return false;

        const auto& eth = buffers_[1];
        const auto& sol = buffers_[2];
        if (eth.empty() || sol.empty()) return false;
        if ((int)eth.size() < MIN_BTC_SAMPLES) return false;

        int64_t now_ms = eth.back().ts_ms;
        static constexpr int64_t ETH_SOL_LOOKBACK_MS  = 80;
        static constexpr double  ETH_MOVE_THRESHOLD_BP = 10.0;

        double eth_ref = 0.0;
        int eth_count = 0;
        for (auto it = eth.rbegin(); it != eth.rend(); ++it) {
            if (now_ms - it->ts_ms > ETH_SOL_LOOKBACK_MS) break;
            eth_ref = it->price;
            eth_count++;
        }
        if (eth_count < MIN_BTC_SAMPLES || eth_ref == 0.0) return false;

        double eth_now   = eth.back().price;
        double eth_delta = (eth_now - eth_ref) / eth_ref * 10000.0;
        if (std::fabs(eth_delta) < ETH_MOVE_THRESHOLD_BP) return false;

        double sol_ref = 0.0;
        for (auto it = sol.rbegin(); it != sol.rend(); ++it) {
            if (now_ms - it->ts_ms > ETH_SOL_LOOKBACK_MS) break;
            sol_ref = it->price;
        }
        if (sol_ref == 0.0) return false;

        double sol_now   = sol.back().price;
        double sol_delta = (sol_now - sol_ref) / sol_ref * 10000.0;

        bool same_dir = (eth_delta > 0) == (sol_delta > 0);
        if (same_dir && std::fabs(sol_delta) >= TARGET_MOVED_MAX_BP) return false;

        direction = (eth_delta > 0) ? 1 : -1;
        return true;
    }

    double eth_move_bp() const {
        const auto& eth = buffers_[1];
        if (eth.size() < 2) return 0.0;
        int64_t now_ms = eth.back().ts_ms;
        double ref = 0.0;
        for (auto it = eth.rbegin(); it != eth.rend(); ++it) {
            if (now_ms - it->ts_ms > 80) break;
            ref = it->price;
        }
        if (ref == 0.0) return 0.0;
        return (eth.back().price - ref) / ref * 10000.0;
    }

private:
    std::deque<PricePoint> buffers_[MAX_SYMBOLS];
};

} // namespace chimera

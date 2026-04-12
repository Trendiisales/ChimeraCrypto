#pragma once
#include <cstdint>
#include <cmath>
#include <deque>
#include <algorithm>
#include "core/SymbolIndex.hpp"
#include "config/TradingConfig.hpp"

namespace chimera {

// ============================================================================
// LeadLagEngine -- N-symbol multi-leader lead-lag detector
//
// Leaders:
//   BTC (id=0) -> ETH, SOL, BNB, AVAX, LINK, XRP    (Tier 1)
//   ETH (id=1) -> SOL, BNB, AVAX, LINK, XRP          (Tier 2, NEW)
//   SOL (id=2) -> AVAX, XRP                           (Tier 3, NEW)
//
// MEASURED LATENCY (Tokyo VPS -> Binance AWS Tokyo):
//   WS feed p95: 18-25ms
//   BTC->follower propagation: 50-200ms
//   ETH->follower propagation: 30-100ms (tighter, ETH moves slower)
//   SOL->follower propagation: 20-60ms  (fastest L1->L1 correlation)
//   Remaining edge window: 25-175ms (conservative: 75ms)
//
// SIGNAL: leader moves >= threshold_bp in LOOKBACK_MS
//         AND follower has NOT yet moved TARGET_MOVED_MAX_BP
// NOTE: thresholds read from TradingConfig to stay in sync with tuning.
// ============================================================================

struct PricePoint {
    double  price;
    int64_t ts_ms;
};

// Tier 2: ETH leads these follower ids
static constexpr int ETH_FOLLOWERS[]    = { 2, 3, 4, 5, 6 }; // SOL, BNB, AVAX, LINK, XRP
static constexpr int ETH_FOLLOWERS_N    = 5;

// Tier 3: SOL leads these follower ids
static constexpr int SOL_FOLLOWERS[]    = { 4, 6 };           // AVAX, XRP
static constexpr int SOL_FOLLOWERS_N    = 2;

class LeadLagEngine {
public:
    static constexpr double  MAX_LATENCY_MS        = 80.0;

    // Tier 1: BTC -> alts — read thresholds from TradingConfig (single source of truth)
    static constexpr double  BTC_MOVE_THRESHOLD_BP = TradingConfig::LEADLAG_BTC_THRESHOLD_BP; // 7.0bp
    static constexpr double  TARGET_MOVED_MAX_BP   = TradingConfig::LEADLAG_TARGET_MAX_BP;    // 3.0bp
    static constexpr int64_t LOOKBACK_MS           = 600;  // FIX: 400→600ms — wider window lets sustained 3bp moves accumulate; still well inside 50-200ms propagation window
    static constexpr int     MIN_BTC_SAMPLES       = 3;

    // Tier 2: ETH -> alts
    static constexpr double  ETH_MOVE_THRESHOLD_BP   = TradingConfig::LEADLAG_ETH_SOL_THRESHOLD_BP; // 9.0bp
    static constexpr double  ETH_TARGET_MOVED_MAX_BP = 2.5;
    static constexpr int64_t ETH_LOOKBACK_MS         = 250;  // extended 160->250ms
    static constexpr int     MIN_ETH_SAMPLES         = 3;

    // Tier 3: SOL -> alts
    static constexpr double  SOL_MOVE_THRESHOLD_BP   = 9.0;
    static constexpr double  SOL_TARGET_MOVED_MAX_BP = 2.5;
    static constexpr int64_t SOL_LOOKBACK_MS         = 200;  // extended 120->200ms
    static constexpr int     MIN_SOL_SAMPLES         = 3;

    LeadLagEngine() {}

    void update_price(int symbol_id, double price, int64_t now_ms) {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return;
        auto& buf = buffers_[symbol_id];
        buf.push_back({price, now_ms});
        while (!buf.empty() && now_ms - buf.front().ts_ms > 1000)  // extended to cover 300ms lookback
            buf.pop_front();
    }

    // -----------------------------------------------------------------------
    // Tier 1: BTC -> target_id
    // -----------------------------------------------------------------------
    bool check_signal(int target_id, double latency_ms, int& direction) const {
        if (target_id == 0) return false;
        if (target_id < 0 || target_id >= MAX_SYMBOLS) return false;
        if (latency_ms > MAX_LATENCY_MS) return false;

        const auto& btc = buffers_[0];
        const auto& tgt = buffers_[target_id];

        if (btc.empty() || tgt.empty()) return false;
        if ((int)btc.size() < MIN_BTC_SAMPLES) return false;

        int64_t now_ms = btc.back().ts_ms;

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

    // -----------------------------------------------------------------------
    // Tier 2: ETH (id=1) -> target_id  (SOL, BNB, AVAX, LINK, POL)
    // -----------------------------------------------------------------------
    bool check_signal_eth_lead(int target_id, double latency_ms, int& direction) const {
        if (target_id <= 1) return false; // ETH can't follow itself or BTC
        if (target_id < 0 || target_id >= MAX_SYMBOLS) return false;
        if (latency_ms > MAX_LATENCY_MS) return false;

        // Verify target is a valid ETH follower
        bool valid_follower = false;
        for (int i = 0; i < ETH_FOLLOWERS_N; ++i)
            if (ETH_FOLLOWERS[i] == target_id) { valid_follower = true; break; }
        if (!valid_follower) return false;

        const auto& eth = buffers_[1];
        const auto& tgt = buffers_[target_id];

        if (eth.empty() || tgt.empty()) return false;
        if ((int)eth.size() < MIN_ETH_SAMPLES) return false;

        int64_t now_ms = eth.back().ts_ms;

        double eth_ref = 0.0;
        int eth_count = 0;
        for (auto it = eth.rbegin(); it != eth.rend(); ++it) {
            if (now_ms - it->ts_ms > ETH_LOOKBACK_MS) break;
            eth_ref = it->price;
            eth_count++;
        }
        if (eth_count < MIN_ETH_SAMPLES || eth_ref == 0.0) return false;

        double eth_now   = eth.back().price;
        double eth_delta = (eth_now - eth_ref) / eth_ref * 10000.0;
        if (std::fabs(eth_delta) < ETH_MOVE_THRESHOLD_BP) return false;

        double tgt_ref = 0.0;
        for (auto it = tgt.rbegin(); it != tgt.rend(); ++it) {
            if (now_ms - it->ts_ms > ETH_LOOKBACK_MS) break;
            tgt_ref = it->price;
        }
        if (tgt_ref == 0.0) return false;

        double tgt_now   = tgt.back().price;
        double tgt_delta = (tgt_now - tgt_ref) / tgt_ref * 10000.0;

        bool same_dir = (eth_delta > 0) == (tgt_delta > 0);
        if (same_dir && std::fabs(tgt_delta) >= ETH_TARGET_MOVED_MAX_BP) return false;

        direction = (eth_delta > 0) ? 1 : -1;
        return true;
    }

    double eth_move_bp() const {
        const auto& eth = buffers_[1];
        if (eth.size() < 2) return 0.0;
        int64_t now_ms = eth.back().ts_ms;
        double ref = 0.0;
        for (auto it = eth.rbegin(); it != eth.rend(); ++it) {
            if (now_ms - it->ts_ms > ETH_LOOKBACK_MS) break;
            ref = it->price;
        }
        if (ref == 0.0) return 0.0;
        return (eth.back().price - ref) / ref * 10000.0;
    }

    // -----------------------------------------------------------------------
    // Tier 3: SOL (id=2) -> AVAX (id=4), POL (id=6)
    // -----------------------------------------------------------------------
    bool check_signal_sol_lead(int target_id, double latency_ms, int& direction) const {
        if (target_id <= 2) return false;
        if (target_id < 0 || target_id >= MAX_SYMBOLS) return false;
        if (latency_ms > MAX_LATENCY_MS) return false;

        bool valid_follower = false;
        for (int i = 0; i < SOL_FOLLOWERS_N; ++i)
            if (SOL_FOLLOWERS[i] == target_id) { valid_follower = true; break; }
        if (!valid_follower) return false;

        const auto& sol = buffers_[2];
        const auto& tgt = buffers_[target_id];

        if (sol.empty() || tgt.empty()) return false;
        if ((int)sol.size() < MIN_SOL_SAMPLES) return false;

        int64_t now_ms = sol.back().ts_ms;

        double sol_ref = 0.0;
        int sol_count = 0;
        for (auto it = sol.rbegin(); it != sol.rend(); ++it) {
            if (now_ms - it->ts_ms > SOL_LOOKBACK_MS) break;
            sol_ref = it->price;
            sol_count++;
        }
        if (sol_count < MIN_SOL_SAMPLES || sol_ref == 0.0) return false;

        double sol_now   = sol.back().price;
        double sol_delta = (sol_now - sol_ref) / sol_ref * 10000.0;
        if (std::fabs(sol_delta) < SOL_MOVE_THRESHOLD_BP) return false;

        double tgt_ref = 0.0;
        for (auto it = tgt.rbegin(); it != tgt.rend(); ++it) {
            if (now_ms - it->ts_ms > SOL_LOOKBACK_MS) break;
            tgt_ref = it->price;
        }
        if (tgt_ref == 0.0) return false;

        double tgt_now   = tgt.back().price;
        double tgt_delta = (tgt_now - tgt_ref) / tgt_ref * 10000.0;

        bool same_dir = (sol_delta > 0) == (tgt_delta > 0);
        if (same_dir && std::fabs(tgt_delta) >= SOL_TARGET_MOVED_MAX_BP) return false;

        direction = (sol_delta > 0) ? 1 : -1;
        return true;
    }

    double sol_move_bp() const {
        const auto& sol = buffers_[2];
        if (sol.size() < 2) return 0.0;
        int64_t now_ms = sol.back().ts_ms;
        double ref = 0.0;
        for (auto it = sol.rbegin(); it != sol.rend(); ++it) {
            if (now_ms - it->ts_ms > SOL_LOOKBACK_MS) break;
            ref = it->price;
        }
        if (ref == 0.0) return 0.0;
        return (sol.back().price - ref) / ref * 10000.0;
    }

    // -----------------------------------------------------------------------
    // Legacy: ETH (id=1) -> SOL (id=2) -- kept for backward compat
    // -----------------------------------------------------------------------
    bool check_signal_eth_sol(double latency_ms, int& direction) const {
        return check_signal_eth_lead(2, latency_ms, direction);
    }

private:
    std::deque<PricePoint> buffers_[MAX_SYMBOLS];
};

} // namespace chimera

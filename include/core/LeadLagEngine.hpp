#pragma once
#include <cstdint>
#include <cmath>
#include <deque>
#include <algorithm>

namespace chimera {

// ============================================================================
// LeadLagEngine
// ============================================================================
//
// MEASURED LATENCY PROFILE (Tokyo VPS → Binance AWS Tokyo):
//   WS feed p95: 18-25ms
//   TCP connect: 4-15ms
//   One-way clock offset: ~18ms
//   Our hard limit: 50ms | lead-lag max: 35ms
//
// EDGE:
//   BTC moves first, ETH and SOL follow 50-200ms later (structural alpha).
//   Our feed latency is 18-25ms p95, so we receive BTC's move at T+20ms.
//   ETH/SOL propagation lag is 50-200ms → we have 25-175ms of remaining window.
//   Conservative: assume 75ms remaining → enough to enter before ETH catches up.
//
// MINIMUM THRESHOLDS (calibrated to beat 10bp round-trip cost):
//   BTC must move >= 12bp in the lookback window  (signal is real, not noise)
//   Target symbol must have moved < 4bp yet       (propagation not yet complete)
//   Lookback window: 100ms                        (BTC leading edge detection)
//
// ============================================================================

struct PricePoint {
    double  price;
    int64_t ts_ms;
};

class LeadLagEngine {
public:
    // -----------------------------------------------------------------------
    // LATENCY LIMITS - calibrated to Tokyo VPS measurements
    // -----------------------------------------------------------------------
    // Lead-lag edge window: BTC moves at T=0, ETH/SOL lag 50-200ms
    // We receive BTC move at T+20ms (our WS latency p95)
    // Remaining edge window: 30-180ms — enough to enter before ETH propagates
    // Max latency to still trade: 35ms (beyond this the window is too small)
    static constexpr double MAX_LATENCY_MS        = 35.0;

    // BTC movement threshold to trigger lead-lag signal
    // 12bp on BTC = $102 move on $85k — genuine institutional tick, not noise
    static constexpr double BTC_MOVE_THRESHOLD_BP = 12.0;

    // Target symbol max move: if ETH/SOL already moved 4bp, edge is consumed
    static constexpr double TARGET_MOVED_MAX_BP   = 4.0;

    // Lookback window for detecting BTC's leading move
    // 100ms captures the initial impulse before propagation begins
    static constexpr int64_t LOOKBACK_MS          = 100;

    // Minimum BTC samples in lookback window to be confident
    static constexpr int MIN_BTC_SAMPLES          = 3;

    LeadLagEngine() {}

    // Call on every tick for every symbol
    void update_price(int symbol_id, double price, int64_t now_ms) {
        auto& buf = buffers_[symbol_id];
        buf.push_back({price, now_ms});
        // Keep only last 500ms of data
        while (!buf.empty() && now_ms - buf.front().ts_ms > 500) {
            buf.pop_front();
        }
    }

    // -----------------------------------------------------------------------
    // check_signal
    //
    // Returns true if there is a lead-lag opportunity on target_id.
    // direction: +1 = LONG, -1 = SHORT (SHORT not traded yet — caller decides)
    //
    // Called from BalancedEngine::check_leadlag() for ETH (id=1) and SOL (id=2)
    // -----------------------------------------------------------------------
    bool check_signal(int target_id, double latency_ms, int& direction) const {
        // LATENCY GATE: beyond 35ms the edge window is too small
        if (latency_ms > MAX_LATENCY_MS) return false;

        const auto& btc = buffers_[0];
        const auto& tgt = buffers_[target_id];

        if (btc.empty() || tgt.empty()) return false;
        if ((int)btc.size() < MIN_BTC_SAMPLES) return false;

        int64_t now_ms = btc.back().ts_ms;

        // --- BTC move in lookback window ---
        double btc_ref = 0.0;
        int    btc_count = 0;
        for (auto it = btc.rbegin(); it != btc.rend(); ++it) {
            if (now_ms - it->ts_ms > LOOKBACK_MS) break;
            if (btc_ref == 0.0) btc_ref = it->price; // oldest in window = baseline
            else btc_ref = it->price;
            btc_count++;
        }
        if (btc_count < MIN_BTC_SAMPLES || btc_ref == 0.0) return false;

        double btc_now   = btc.back().price;
        double btc_delta = (btc_now - btc_ref) / btc_ref * 10000.0; // bp

        // BTC must have moved at least 12bp in the window
        if (std::fabs(btc_delta) < BTC_MOVE_THRESHOLD_BP) return false;

        // --- Target symbol must NOT have moved much yet ---
        double tgt_ref = 0.0;
        for (auto it = tgt.rbegin(); it != tgt.rend(); ++it) {
            if (now_ms - it->ts_ms > LOOKBACK_MS) break;
            tgt_ref = it->price;
        }
        if (tgt_ref == 0.0) return false;

        double tgt_now   = tgt.back().price;
        double tgt_delta = (tgt_now - tgt_ref) / tgt_ref * 10000.0; // bp

        // If target already moved 4bp+ in BTC's direction, edge is consumed
        // Use directional check: if both moving the same way, edge is gone
        bool same_direction = (btc_delta > 0) == (tgt_delta > 0);
        if (same_direction && std::fabs(tgt_delta) >= TARGET_MOVED_MAX_BP) return false;

        direction = (btc_delta > 0) ? 1 : -1;
        return true;
    }

    // How far BTC has moved in the last lookback window (bp)
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

private:
    // Ring buffers: [0]=BTC, [1]=ETH, [2]=SOL
    std::deque<PricePoint> buffers_[3];
};

} // namespace chimera

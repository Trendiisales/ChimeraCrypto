#pragma once
// ============================================================================
// LimitOrderManager
// ============================================================================
// Implements maker-order entry for the Chimera engine.
//
// COST IMPACT:
//   Taker entry:      fee=4bp/side → not acceptable for this strategy set
//   Maker entry:      fee/rebate profile keeps entry cost low enough to study
//                     real queue behavior without assuming instant taker fills
//
// HOW IT WORKS:
//   1. Signal fires → enter_pending() posts a limit bid at limit_price
//   2. Every tick → update() checks whether the limit was filled:
//      - Fill condition: ask_price <= limit_price (a seller would hit our bid)
//      - Stale condition: price moved away by STALE_DISTANCE_BP or timeout expired
//   3. On fill → returns FILLED with actual fill price
//   4. On stale/timeout → returns CANCELLED, engine tries again next signal
//
// LIMIT PLACEMENT PROFILES:
//   0 PASSIVE_BID:     bid price
//   1 FAST_INSIDE:     inside spread, closer to ask but still maker-safe
//   2 MID_INSIDE:      inside spread, moderate aggression
//   3 LEAD_INSIDE:     tighter lead-lag profile
//   4 AGGRESSIVE_MAKER: near-ask inside spread for momentum layers
//
// SHADOW MODE:
//   No real orders are sent. Fill detection uses ask_price crossing our bid.
//   This slightly understates fills (real exchange fills at bid exactly,
//   we require ask <= bid which is the same thing). Conservative and correct.
// ============================================================================
#include <cstdint>
#include <cstdio>
#include "config/TradingConfig.hpp"

namespace chimera {

enum class LimitStatus {
    IDLE,       // No pending order
    PENDING,    // Limit posted, waiting for fill
    FILLED,     // Filled — use fill_price as entry
    CANCELLED   // Timed out or price moved away — abandon
};

struct LimitOrder {
    LimitStatus status      = LimitStatus::IDLE;
    double      limit_price = 0.0;   // Our posted bid
    double      fill_price  = 0.0;   // Actual fill (ask that crossed us)
    int64_t     posted_ts   = 0;     // When we posted
    int64_t     timeout_ms  = 0;     // Max wait before cancelling
    double      stale_bp    = 0.0;   // Cancel if ask rises this many bp above limit
    int         layer_id    = 0;     // Which strategy posted this
};

class LimitOrderManager {
public:
    // -----------------------------------------------------------------------
    // enter_pending
    // Call when signal fires. Computes and stores the limit price.
    //
    // bid, ask: current best bid/ask from bookTicker
    // layer:    determines placement aggressiveness and timeout
    // ts:       current timestamp ms
    // -----------------------------------------------------------------------
    void enter_pending(int layer_id, double bid, double ask,
                       int64_t ts) {
        double spread    = ask - bid;
        double mid       = (bid + ask) / 2.0;
        double limit_px  = 0.0;
        int64_t timeout  = 0;
        double stale_bp  = 0.0;

        if (layer_id == 0) {  // PASSIVE_BID
            limit_px  = bid;
            timeout   = TradingConfig::MAKER_IMBALANCE_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP;
        }
        else if (layer_id == 1) {  // FAST_INSIDE
            limit_px  = bid + 0.75 * spread;
            timeout   = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP * 2.0;
        }
        else if (layer_id == 2) {  // MID_INSIDE
            limit_px  = mid - 0.15 * spread;
            timeout   = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP;
        }
        else if (layer_id == 3) {  // LEAD_INSIDE
            limit_px  = bid + 0.85 * spread;
            timeout   = TradingConfig::MAKER_LEADLAG_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP * 2.0;
        }
        else if (layer_id == 4) {
            limit_px  = ask - 0.1 * spread;
            timeout   = 300;   // 300ms: LEADLAG edge window ~80ms, 3x buffer
            stale_bp  = 2.0;   // cancel if ask rises 2bp — move has started without us
        }
        else {
            limit_px  = mid - 0.3 * spread;
            timeout   = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP;
        }

        // Safety: never post below bid - 3bp (would cross spread on post = taker)
        double min_limit = bid * (1.0 - 0.0003);
        if (limit_px < min_limit) limit_px = bid;

        order_.status      = LimitStatus::PENDING;
        order_.limit_price = limit_px;
        order_.fill_price  = 0.0;
        order_.posted_ts   = ts;
        order_.timeout_ms  = timeout;
        order_.stale_bp    = stale_bp;
        order_.layer_id    = layer_id;

        std::printf("[LIMIT-POST] layer=%d | bid=%.4f | limit=%.4f | spread_bp=%.2f | timeout=%ldms\n",
            layer_id, bid, limit_px,
            (ask - bid) / mid * 10000.0, timeout);
        std::fflush(stdout);
    }

    // -----------------------------------------------------------------------
    // update — call on every tick while status == PENDING
    //
    // ask:      current best ask price
    // ts:       current timestamp ms
    //
    // Returns current status. Caller checks:
    //   FILLED    → use order_.fill_price as entry_price
    //   CANCELLED → discard, re-evaluate signal next tick
    //   PENDING   → continue waiting
    // -----------------------------------------------------------------------
    LimitStatus update(double ask, double bid, int64_t ts) {
        if (order_.status != LimitStatus::PENDING) return order_.status;

        // FILL CHECK: ask <= our limit means a market sell would fill us
        // We model fill at our limit_price (conservative — real fills at bid)
        if (ask <= order_.limit_price) {
            order_.fill_price  = order_.limit_price;
            order_.status      = LimitStatus::FILLED;
            int64_t wait_ms    = ts - order_.posted_ts;
            std::printf("[LIMIT-FILL] limit=%.4f | ask=%.4f | wait=%ldms\n",
                order_.limit_price, ask, wait_ms);
            std::fflush(stdout);
            return LimitStatus::FILLED;
        }

        // STALE CHECK: ask has risen too far above our limit — we missed the move
        double ask_above_bp = (ask - order_.limit_price) / order_.limit_price * 10000.0;
        if (ask_above_bp > order_.stale_bp) {
            order_.status = LimitStatus::CANCELLED;
            std::printf("[LIMIT-STALE] limit=%.4f | ask=%.4f | drift=%.2fbp > %.2fbp\n",
                order_.limit_price, ask, ask_above_bp, order_.stale_bp);
            std::fflush(stdout);
            return LimitStatus::CANCELLED;
        }

        // TIMEOUT CHECK
        if (ts - order_.posted_ts > order_.timeout_ms) {
            order_.status = LimitStatus::CANCELLED;
            std::printf("[LIMIT-TIMEOUT] limit=%.4f | waited=%ldms\n",
                order_.limit_price, order_.timeout_ms);
            std::fflush(stdout);
            return LimitStatus::CANCELLED;
        }

        return LimitStatus::PENDING;
    }

    void cancel() {
        if (order_.status == LimitStatus::PENDING) {
            order_.status = LimitStatus::CANCELLED;
        }
    }

    void reset() { order_ = LimitOrder{}; }

    const LimitOrder& order() const { return order_; }
    LimitStatus       status() const { return order_.status; }
    double            fill_price() const { return order_.fill_price; }
    bool              pending() const { return order_.status == LimitStatus::PENDING; }

private:
    LimitOrder order_;
};

} // namespace chimera

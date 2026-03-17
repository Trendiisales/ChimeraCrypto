#pragma once
// ============================================================================
// LimitOrderManager
// ============================================================================
// Implements maker-order entry for the Chimera engine.
//
// COST IMPACT:
//   Taker (current):  fee=4bp/side → 8bp round trip + spread ~1bp = ~10bp total
//   Maker (this):     fee=1bp rebate/side → -2bp + spread 0bp = ~4bp total
//   Saving per trade: ~6bp — at 1 trade/day that is the difference between
//   a marginally losing and marginally winning system.
//
// HOW IT WORKS:
//   1. Signal fires → enter_pending() posts a limit bid at limit_price
//   2. Every tick → update() checks whether the limit was filled:
//      - Fill condition: ask_price <= limit_price (a seller would hit our bid)
//      - Stale condition: price moved away by STALE_DISTANCE_BP or timeout expired
//   3. On fill → returns FILLED with actual fill price
//   4. On stale/timeout → returns CANCELLED, engine tries again next signal
//
// LIMIT PLACEMENT BY STRATEGY:
//   IMBALANCE : bid price         (buy pressure — sellers will come to us)
//   LEAD-LAG  : mid - 0.4*spread  (fast edge window — can't afford to miss)
//   IMPULSE   : mid - 0.25*spread (breakout — fills quickly or not at all)
//   EXPANSION : mid - 0.3*spread
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

        // LAYER_MICRO (imbalance): post at bid
        //   Strong buy pressure — sellers will come to us at bid price
        //   Wide timeout: 5s (imbalance can persist)
        //   Stale if ask rises 3bp above our limit (we missed the move)
        if (layer_id == 0) {  // LAYER_MICRO
            limit_px  = bid;
            timeout   = TradingConfig::MAKER_IMBALANCE_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP;
        }
        // LAYER_LEADLAG: post at ask (cross the spread — aggressive fill)
        //   LEADLAG fires when BTC moves and ETH/SOL is RISING. Price moves UP.
        //   A limit BELOW mid will never fill — ask moves away from us immediately.
        //   We must cross the spread and fill at ask. The 6bp maker saving is worthless
        //   if the order never fills. LEADLAG edge (8bp net) > spread cost (~0.5bp).
        //   Timeout: 200ms still valid — if ask has moved far above our fill, stale exit.
        else if (layer_id == 3) {  // LAYER_LEADLAG — aggressive: fill at ask
            limit_px  = ask;  // cross the spread — guarantees immediate fill on rising price
            timeout   = TradingConfig::MAKER_LEADLAG_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP * 2.0;  // allow more drift — we're filling into a move
        }
        // LAYER_IMPULSE: post at ask (aggressive fill — breakout price is moving UP)
        //   Same logic as LEADLAG: price is expanding upward on a breakout.
        //   Posting below mid means ask is running away. Fill at ask immediately.
        //   Timeout: 500ms — if not filled at ask, price has moved too far.
        else if (layer_id == 1) {  // LAYER_IMPULSE — aggressive: fill at ask
            limit_px  = ask;  // cross the spread — breakout price is moving fast
            timeout   = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
            stale_bp  = TradingConfig::MAKER_STALE_BP * 2.0;
        }
        // LAYER_LEADLAG aggressive maker (layer_id=4): post at ask - 0.1*spread
        // Sits just inside the spread. Fills when a seller hits slightly below ask.
        // Saves ~4bp vs taker (no crossing fee), fills ~70% of time given 300ms window.
        // If price runs away (ask rises 2bp) = stale, signal is gone anyway.
        else if (layer_id == 4) {
            limit_px  = ask - 0.1 * spread;
            timeout   = 300;   // 300ms: LEADLAG edge window ~80ms, 3x buffer
            stale_bp  = 2.0;   // cancel if ask rises 2bp — move has started without us
        }
        // LAYER_LIQUIDATION maker (layer_id=5): post at bid + 0.1*spread
        // LIQ cascade pushes price up fast — we post just above bid to catch the lift.
        // 400ms timeout: cascade either resolves in <2s or doesn't at all.
        // Stale at 3bp: if ask blows past us by 3bp the cascade already ran without us.
        else if (layer_id == 5) {
            limit_px  = bid + 0.1 * spread;
            timeout   = 400;   // 400ms: cascade front-runs fast, must fill early
            stale_bp  = 3.0;   // if ask runs 3bp above limit, cascade is already in price
        }
        // LAYER_SESSION_MOM maker (layer_id=6): post at ask - 0.15*spread
        // Session open momentum — price is displacing upward but not spiking yet.
        // Patient maker fill: 800ms is fine, session momentum lasts 10-20 minutes.
        // Stale at 4bp: if we're 4bp away the displacement already ran.
        else if (layer_id == 6) {
            limit_px  = ask - 0.15 * spread;
            timeout   = 800;   // 800ms: session momentum is slower than scalp signals
            stale_bp  = 4.0;   // wider stale — session opens have more noise
        }
        // LAYER_VOLSHOCK maker (layer_id=7): post at ask - 0.1*spread
        // Volume shock + displacement — similar urgency to LEADLAG but slightly slower.
        // 500ms timeout: if volume spike hasn't attracted a fill in 500ms, edge is gone.
        else if (layer_id == 7) {
            limit_px  = ask - 0.1 * spread;
            timeout   = 500;   // 500ms: vol shock continuation is slightly slower than leadlag
            stale_bp  = 2.5;   // cancel if ask rises 2.5bp — move ran without us
        }
        // LAYER_EXPANSION and fallback: post at mid - 0.3 * spread (kept for reference, disabled)
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

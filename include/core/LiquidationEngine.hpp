#pragma once
// ============================================================================
// LiquidationEngine.hpp — Converts liquidation events to spot long signals
//
// LOGIC:
//   1. Short liquidation arrives (Binance perp: forced BUY on perp)
//   2. Perp price pumps immediately
//   3. Spot follows perp up within 50-200ms (same lead-lag mechanism)
//   4. We buy spot before it catches up
//
// SIGNAL CONDITIONS:
//   - is_short_liq = true (short being closed = buy pressure)
//   - notional_usd >= MIN_NOTIONAL (filter noise, only meaningful liquidations)
//   - spot has NOT yet moved >= SPOT_MOVED_MAX_BP (still catching up)
//   - no position already open on this symbol
//   - cooldown window not active (prevent stacking on cascade)
//
// SIZING: handled by BalancedEngine eng_mult (same as LEADLAG)
// ============================================================================

#include <cstdint>
#include <cmath>
#include <array>
#include <chrono>
#include <cstdio>
#include "core/SymbolIndex.hpp"
#include "core/LiquidationFeed.hpp"
#include "config/TradingConfig.hpp"

namespace chimera {

struct LiquidationSignal {
    int     symbol_id   = -1;
    bool    valid       = false;
    double  liq_price   = 0.0;    // perp liquidation price
    double  notional    = 0.0;    // USD notional of the liquidation
    int64_t ts_ms       = 0;
};

class LiquidationEngine {
public:
    LiquidationEngine() {
        pending_.fill({});
        last_entry_ts_.fill(0);
        spot_price_at_liq_.fill(0.0);
    }

    // Called from LiquidationFeed callback (separate thread) — thread safe via atomic
    void on_liquidation(const LiquidationEvent& ev) {
        if (ev.symbol_id < 0 || ev.symbol_id >= MAX_SYMBOLS) return;
        if (!ev.is_short_liq) return;
        if (ev.notional_usd < TradingConfig::LIQ_MIN_NOTIONAL_USD) return;

        int id = ev.symbol_id;
        int64_t now = ev.ts_ms;

        // Cooldown — don't stack entries on the same symbol
        if (now - last_entry_ts_[id] < TradingConfig::LIQ_COOLDOWN_MS) {
            std::printf("[LIQ-ENGINE] %s cooldown active, skipping $%.0f liq\n",
                sym_short(id), ev.notional_usd);
            std::fflush(stdout);
            return;
        }

        std::printf("[LIQ-ENGINE] SHORT LIQ %s | notional=$%.0f | price=%.2f | queuing signal\n",
            sym_short(id), ev.notional_usd, ev.price);
        std::fflush(stdout);

        // Store pending signal — check_signal() will validate spot hasn't moved yet
        pending_[id].symbol_id  = id;
        pending_[id].valid      = true;
        pending_[id].liq_price  = ev.price;
        pending_[id].notional   = ev.notional_usd;
        pending_[id].ts_ms      = now;

        // Record spot price at liquidation time — set by check_signal on next tick
        spot_price_at_liq_[id]  = 0.0;  // Will be filled on first check_signal call
    }

    // Called from main tick loop (BalancedEngine thread)
    // Returns true if this symbol has a pending liquidation signal that's still valid
    bool check_signal(int id, double spot_price, int64_t now_ms, double latency_ms) {
        if (id < 0 || id >= MAX_SYMBOLS) return false;
        auto& sig = pending_[id];
        if (!sig.valid) return false;

        // Signal expired
        if (now_ms - sig.ts_ms > TradingConfig::LIQ_SIGNAL_WINDOW_MS) {
            std::printf("[LIQ-ENGINE] %s signal expired (%lldms old)\n",
                sym_short(id), (long long)(now_ms - sig.ts_ms));
            std::fflush(stdout);
            sig.valid = false;
            return false;
        }

        // Latency gate — same as LEADLAG
        if (latency_ms > TradingConfig::LATENCY_LEADLAG_MAX_MS) {
            return false;
        }

        // Record spot price at signal time (first tick after liquidation)
        if (spot_price_at_liq_[id] == 0.0) {
            spot_price_at_liq_[id] = spot_price;
        }

        // Spot must NOT have already moved too much (we'd be chasing)
        double spot_moved_bp = (spot_price - spot_price_at_liq_[id])
                               / spot_price_at_liq_[id] * 10000.0;

        if (spot_moved_bp >= TradingConfig::LIQ_SPOT_MOVED_MAX_BP) {
            std::printf("[LIQ-ENGINE] %s spot already moved %.2fbp — too late\n",
                sym_short(id), spot_moved_bp);
            std::fflush(stdout);
            sig.valid = false;
            return false;
        }

        return true;
    }

    // Called when we successfully enter a trade — consume the signal + set cooldown
    void consume_signal(int id, int64_t now_ms) {
        if (id < 0 || id >= MAX_SYMBOLS) return;
        pending_[id].valid    = false;
        last_entry_ts_[id]    = now_ms;
        spot_price_at_liq_[id] = 0.0;
    }

    double get_notional(int id) const {
        if (id < 0 || id >= MAX_SYMBOLS) return 0.0;
        return pending_[id].notional;
    }

    double spot_move_bp(int id, double spot_price) const {
        if (id < 0 || id >= MAX_SYMBOLS) return 0.0;
        const double baseline = spot_price_at_liq_[id];
        if (baseline <= 0.0) return 0.0;
        return (spot_price - baseline) / baseline * 10000.0;
    }

private:
    std::array<LiquidationSignal, MAX_SYMBOLS> pending_;
    std::array<int64_t, MAX_SYMBOLS>           last_entry_ts_;
    std::array<double,  MAX_SYMBOLS>           spot_price_at_liq_;
};

} // namespace chimera

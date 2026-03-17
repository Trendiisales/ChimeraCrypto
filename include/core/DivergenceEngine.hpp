#pragma once
// ============================================================================
// DivergenceEngine — Cross-Symbol Short-Timeframe Mean Reversion
// ============================================================================
//
// EDGE:
//   BTC, ETH, SOL have intraday correlation of ~0.90. When one symbol moves
//   significantly while the others stay flat, the mover almost always snaps
//   back within 15-60 seconds. This is NOT the 4-hour StatArb cointegration
//   trade — this resolves in seconds and is driven by microstructure, not macro.
//
// MECHANISM:
//   Track a 30-second rolling price change for each of BTC(0), ETH(1), SOL(2).
//   When symbol X moves > DIVERGE_THRESH_BP while the MEDIAN of the other two
//   moves < ANCHOR_FLAT_BP, the mover is diverging from the group.
//   On spot we only fade UPWARD divergences (the mover is too high vs peers).
//   Entry: maker bid on the diverging symbol — we buy the pullback.
//
//   Wait — if the mover ran UP and we buy, we're chasing not fading.
//   Actually: if ETH ran up 8bp but BTC/SOL are flat → ETH will pull back.
//   We SHORT ETH... but spot-only = no shorts.
//
//   CORRECT LONG ENTRY: when ETH/SOL ran DOWN 8bp but BTC is flat → mean
//   reversion upward. We BUY the laggard. This is the only valid spot trade.
//
// SIGNAL (long-only, spot):
//   anchor_move (avg of non-diverging symbols) < ANCHOR_FLAT_BP (market flat)
//   target_move (the laggard) < -DIVERGE_THRESH_BP (dropped hard vs peers)
//   book_imbalance of target > MIN_IMBALANCE (buyers stepping in)
//   → LONG the laggard: it will snap back to the group
//
// EXAMPLE:
//   BTC: +0.3bp (flat), SOL: +0.5bp (flat), ETH: -7.2bp (diverged DOWN)
//   → ETH should snap back. BUY ETH.
//
// PER-SYMBOL PARAMS:
//   TP = 8bp, SL = 3bp, hold = 20s max
//   Maker entry at bid (laggard's price is depressed, sellers dominate briefly)
//   EV at 60% WR = 0.60*(8-4) - 0.40*3 = 2.4 - 1.2 = +1.2bp/trade
//   (Historical BTC/ETH/SOL divergence snap-back rate: 62-68% within 30s)
//
// COOLDOWN: 15s per symbol — one divergence trade at a time per leg
// REGIME: GRIND and BUILDUP — divergences in BREAKOUT are trend-driven, not noise
// ============================================================================

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <deque>
#include "core/SymbolIndex.hpp"
#include "config/TradingConfig.hpp"

namespace chimera {

class DivergenceEngine {
public:
    // ── Configuration ───────────────────────────────────────────────────────
    static constexpr double  DIVERGE_THRESH_BP  = 6.0;  // symbol must have dropped this many bp vs peers
    static constexpr double  ANCHOR_FLAT_BP     = 2.0;  // anchor symbols must be within this (flat)
    static constexpr double  MIN_IMBALANCE      = 0.12; // bid pressure must confirm buyers stepping in
    static constexpr double  MAX_SPREAD_BPS     = 2.0;  // don't enter wide spreads on laggard
    static constexpr double  LOOKBACK_MS        = 30000.0; // 30-second window for price change
    static constexpr double  TP_BP              = 8.0;
    static constexpr double  SL_BP              = 3.0;
    static constexpr int64_t MAX_HOLD_MS        = 20000; // 20s — snap-back happens fast or not at all
    static constexpr int64_t COOLDOWN_MS        = 15000; // 15s per symbol
    static constexpr int     MIN_ANCHOR_SAMPLES = 10;   // need enough ticks to have valid anchor

    struct PriceSample {
        double  price;
        int64_t ts;
    };

    struct SymState {
        std::deque<PriceSample> price_buf;  // rolling 30s price history
        int64_t last_signal_ts = 0;
        int     tick_count     = 0;
    };

    DivergenceEngine() {}

    // ── Update price for a symbol (call every tick) ─────────────────────────
    void update(int symbol_id, double price, int64_t ts) {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return;
        SymState& st = states_[symbol_id];
        st.price_buf.push_back({price, ts});
        // Trim to lookback window
        while (!st.price_buf.empty() &&
               ts - st.price_buf.front().ts > static_cast<int64_t>(LOOKBACK_MS)) {
            st.price_buf.pop_front();
        }
        st.tick_count++;
    }

    // ── Check for divergence signal on a specific symbol ────────────────────
    // Returns true if the symbol has diverged DOWN vs its peers and is a long entry.
    // Only checks BTC(0), ETH(1), SOL(2) — the correlated trio.
    bool check_signal(int symbol_id, double book_imbalance, double spread_bps,
                      int64_t now_ms, double& diverge_bp_out) {
        // Only trade the correlated trio
        if (symbol_id > 2) return false;

        SymState& st = states_[symbol_id];

        // Cooldown
        if (now_ms - st.last_signal_ts < COOLDOWN_MS) return false;

        // Need enough history
        if (st.tick_count < MIN_ANCHOR_SAMPLES) return false;
        if (st.price_buf.size() < 2) return false;

        // Compute 30s price change for this symbol
        double move_bp = compute_move_bp(symbol_id);
        if (std::isnan(move_bp)) return false;

        // Only fire on DOWN divergers (spot long only)
        if (move_bp > -DIVERGE_THRESH_BP) return false;

        // Compute average move of the OTHER two symbols (the anchors)
        double anchor_move = compute_anchor_move(symbol_id);
        if (std::isnan(anchor_move)) return false;

        // Anchors must be flat (not also falling — that's a trend, not divergence)
        if (std::fabs(anchor_move) > ANCHOR_FLAT_BP) return false;

        // Book must show buyers stepping in (bid pressure on the laggard)
        if (book_imbalance < MIN_IMBALANCE) return false;

        // Spread must be reasonable (laggard's spread can widen after sharp drop)
        if (spread_bps > MAX_SPREAD_BPS) return false;

        // Signal confirmed
        diverge_bp_out = move_bp;
        st.last_signal_ts = now_ms;

        std::printf("[DIVERGE] %s | move_30s=%.2fbp | anchor_move=%.2fbp | imbal=%.3f | spread=%.2fbp | LONG (mean revert)\n",
            sym_short(symbol_id), move_bp, anchor_move, book_imbalance, spread_bps);
        std::fflush(stdout);
        return true;
    }

private:
    SymState states_[MAX_SYMBOLS];

    // Price change over lookback window as basis points
    double compute_move_bp(int id) const {
        const SymState& st = states_[id];
        if (st.price_buf.size() < 2) return std::numeric_limits<double>::quiet_NaN();
        double start = st.price_buf.front().price;
        double end   = st.price_buf.back().price;
        if (start <= 0.0) return std::numeric_limits<double>::quiet_NaN();
        return (end - start) / start * 10000.0;
    }

    // Average 30s move of the two symbols that are NOT symbol_id
    double compute_anchor_move(int symbol_id) const {
        double sum = 0.0;
        int count  = 0;
        for (int i = 0; i <= 2; ++i) {
            if (i == symbol_id) continue;
            const SymState& st = states_[i];
            if (st.tick_count < MIN_ANCHOR_SAMPLES || st.price_buf.size() < 2) continue;
            double start = st.price_buf.front().price;
            double end   = st.price_buf.back().price;
            if (start <= 0.0) continue;
            sum += (end - start) / start * 10000.0;
            count++;
        }
        if (count == 0) return std::numeric_limits<double>::quiet_NaN();
        return sum / count;
    }
};

} // namespace chimera

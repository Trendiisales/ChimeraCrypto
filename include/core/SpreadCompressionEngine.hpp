#pragma once
// ============================================================================
// SpreadCompressionEngine — Spread Tightening as Directional Signal
// ============================================================================
//
// EDGE:
//   Market makers widen spreads when uncertain (inventory risk) and tighten
//   them when they have conviction about direction. A spread compression event
//   (wide → tight in 1-3 ticks) means MMs are committing capital — price is
//   about to move in the direction the book is leaning.
//
// MECHANISM:
//   1. Spread was elevated (>WIDE_THRESH bps) for at least MIN_WIDE_TICKS ticks
//   2. Spread compresses to below TIGHT_THRESH bps within COMPRESS_WINDOW ticks
//   3. Book imbalance confirms direction (bid-heavy → long entry)
//   4. Enter maker limit on the bid side — we are buying ahead of the move
//
// WHY MAKER:
//   MMs just tightened — ask will stay near bid for a few ticks.
//   Posting at bid fills quickly (0.5-2s) and costs ~4bp round trip.
//   TP=10bp, SL=3bp → EV = 0.55*6 - 0.45*3 = +1.95bp at 55% WR.
//
// WHAT IT IS NOT:
//   NOT a breakout engine (we are not chasing the move after spread widens)
//   NOT a noise filter (we require WIDE→TIGHT direction, not just tight spread)
//   The key signal is the TRANSITION, not the absolute spread level.
//
// REGIME GATE:
//   GRIND and BUILDUP only. In BREAKOUT, spread compression is too noisy
//   (spreads oscillate wildly during fast moves).
//
// PER-SYMBOL COOLDOWN: 8 seconds — prevents re-entry into same compression
// ============================================================================

#include <cstdint>
#include <cmath>
#include <cstdio>
#include "core/SymbolIndex.hpp"
#include "config/TradingConfig.hpp"

namespace chimera {

class SpreadCompressionEngine {
public:
    // ── Configuration ───────────────────────────────────────────────────────
    static constexpr double  WIDE_THRESH_BPS    = 1.2;  // spread must reach this to count as "wide"
    static constexpr double  TIGHT_THRESH_BPS   = 0.6;  // spread must compress to this to signal
    static constexpr int     MIN_WIDE_TICKS     = 3;    // must have been wide for at least 3 ticks
    static constexpr int     COMPRESS_WINDOW    = 5;    // compression must happen within 5 ticks
    static constexpr double  MIN_IMBALANCE      = 0.15; // book must lean in entry direction
    static constexpr double  MAX_SPREAD_AT_ENTRY= 0.8;  // don't enter if spread bounced back wide
    static constexpr double  TP_BP              = 10.0;
    static constexpr double  SL_BP              =  3.0;
    static constexpr int64_t MAX_HOLD_MS        = 12000; // 12s — compression moves are fast
    static constexpr int64_t COOLDOWN_MS        =  8000; // 8s between entries per symbol

    struct SymState {
        // Spread history for compression detection
        double  spread_history[8] = {};  // ring buffer of recent spread_bps
        int     spread_head       = 0;
        int     spread_count      = 0;

        // Wide phase tracking
        int     wide_tick_count   = 0;   // consecutive ticks above WIDE_THRESH
        bool    was_wide          = false;

        // Cooldown
        int64_t last_signal_ts    = 0;

        // Per-tick prev spread (for detecting the compression direction)
        double  prev_spread       = 0.0;
        bool    spread_init       = false;
    };

    SpreadCompressionEngine() {}

    // ── Main signal check ────────────────────────────────────────────────────
    // Returns true if a compression signal fires.
    // direction: +1 = long (bid-heavy book), -1 = short (not used — spot only)
    bool check_signal(int symbol_id, double spread_bps, double book_imbalance,
                      int64_t now_ms, int& direction) {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return false;
        SymState& st = states_[symbol_id];

        // Cooldown guard
        if (now_ms - st.last_signal_ts < COOLDOWN_MS) return false;

        // Update spread ring buffer
        st.spread_history[st.spread_head % 8] = spread_bps;
        st.spread_head++;
        if (st.spread_count < 8) st.spread_count++;

        // Track wide phase
        if (spread_bps >= WIDE_THRESH_BPS) {
            st.wide_tick_count++;
            if (st.wide_tick_count >= MIN_WIDE_TICKS) st.was_wide = true;
        } else {
            // Spread is no longer wide — check for compression signal
            if (st.was_wide && spread_bps <= TIGHT_THRESH_BPS) {
                // Compression event detected
                // Require book imbalance to confirm direction
                if (book_imbalance >= MIN_IMBALANCE) {
                    // Long signal: spread compressed AND bid-heavy book
                    direction = 1;
                    st.last_signal_ts = now_ms;
                    st.was_wide = false;
                    st.wide_tick_count = 0;
                    std::printf("[SPREAD-COMPRESS] sym=%d | spread=%.2fbp (was wide) | imbal=%.3f | LONG signal\n",
                        symbol_id, spread_bps, book_imbalance);
                    std::fflush(stdout);
                    return true;
                }
            }
            // Reset wide tracking when spread normalises
            if (spread_bps < WIDE_THRESH_BPS * 0.8) {
                st.wide_tick_count = std::max(0, st.wide_tick_count - 1);
                if (st.wide_tick_count == 0) st.was_wide = false;
            }
        }

        st.prev_spread = spread_bps;
        st.spread_init = true;
        return false;
    }

    // Reset cooldown (e.g. if entry was rejected by another gate)
    void reset_cooldown(int symbol_id) {
        if (symbol_id >= 0 && symbol_id < MAX_SYMBOLS)
            states_[symbol_id].last_signal_ts = 0;
    }

private:
    SymState states_[MAX_SYMBOLS];
};

} // namespace chimera

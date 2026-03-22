#pragma once
// ============================================================================
// BasisMomentumEngine.hpp
// Chimera — Perp Basis Momentum Engine (SPOT LONG ONLY)
//
// CORE EDGE:
//   Perp futures price moves before spot. When aggressive buying hits perp,
//   basis (mark - spot) spikes positive. Spot catches up within 50-500ms.
//   We buy spot at the start of that lag window and exit when spot catches up.
//
// SIGNAL: Basis transition — from low/negative → positive spike
//   prev_basis = -5.9bp (shorts crowded, perp discounted)
//   curr_basis = +12bp  (aggressive perp buying just hit)
//   delta      = +17.9bp in one tick = LONG signal
//
// THREE CONFIRMATION GATES (all must be true):
//   1. Basis crossed above ENTRY_THRESHOLD_BP (+5bp)
//   2. Basis delta (change vs prev tick) > DELTA_TRIGGER_BP (+8bp)
//   3. Perp flow ratio > FLOW_CONFIRM (0.25) — perp buy volume dominating
//
// SPOT ONLY = LONG ONLY:
//   Basis collapse (perp falling below spot) = shorts winning.
//   Can't short on spot without margin. We only trade the long side.
//   Basis collapse while in position = EARLY EXIT signal.
//
// COST MODEL (15bp round trip with BNB discount):
//   TP: +35bp gross = +20bp net  (spot catching up to perp)
//   SL: -12bp gross = -27bp net
//   Break-even WR: 27 / (20+27) = 57.4%
//   Expected WR on genuine basis spikes: 55-65% (perp leads spot reliably)
//
// EARLY EXIT:
//   If basis collapses back below EXIT_BASIS_BP while in position,
//   exit immediately — the perp move was absorbed, spot won't follow.
//
// COOLDOWN: 90s per symbol after any exit — prevents chasing
// ============================================================================

#include <cmath>
#include <cstdint>
#include <string>
#include <cstdio>
#include <algorithm>

namespace chimera {

class BasisMomentumEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP  = 15.0; // 7.5bp/side BNB discount

    // Entry gates
    static constexpr double ENTRY_THRESHOLD_BP  = 8.0;  // raised 5->8bp: require bigger spike
    static constexpr double DELTA_TRIGGER_BP    = 12.0; // raised 8->12bp: require sharper spike
    static constexpr double FLOW_CONFIRM        = 0.30; // raised 0.20->0.30: stronger flow required

    // Exit levels
    static constexpr double TARGET_BP           = 2000.0; // hard cap — unlimited, trail always exits first
    static constexpr double TRAIL_ARM_BP        = 20.0;   // start trailing once +20bp profit

    // Dynamic trail distance — tighter as move gets larger to capture more of big runs
    static double trail_distance_bp(double peak_bp) {
        if (peak_bp < 50.0)  return 20.0;
        if (peak_bp < 100.0) return 18.0;
        if (peak_bp < 200.0) return 15.0;
        if (peak_bp < 300.0) return 12.0;
        return 8.0;  // >= 300bp: 8bp trail captures 97%+
    }
    static constexpr double STOP_BP             = 8.0;  // tightened 12->8bp: cut loss faster
    static constexpr double EXIT_BASIS_BP       = -2.0; // early exit: basis collapsed

    // Timing
    static constexpr int64_t MAX_HOLD_MS        = 15000; // 15s — trail will exit; basis collapse exits early
    static constexpr int64_t COOLDOWN_MS        = 90000;// 90s cooldown per symbol

    struct Stats {
        bool   active;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double win_rate;
        double total_pnl_bp;
        int    total_trades;
        // For GUI readiness display
        double basis_now;
        double basis_delta;
    };

    explicit BasisMomentumEngine(const std::string& sym = "") : symbol_(sym) {}

    // Called each tick from QuadEngine
    // basis_bp:    (perp_mark - spot) / spot * 10000
    // flow_ratio:  perp buy/sell EMA ratio (-1..+1)
    // vol_ratio:   spot volatility ratio
    void evaluate(
        double  price,
        double  basis_bp,
        double  flow_ratio,
        double  vol_ratio,
        int64_t ts,
        double  available_R
    ) {
        if (ts < cooldown_until_ms_) return;
        if (available_R < 0.5) return;

        double basis_delta = basis_bp - prev_basis_bp_;
        prev_basis_bp_ = basis_bp;

        if (!pos_active_) {
            // ── ENTRY ─────────────────────────────────────────────────────
            // Gate 1: basis is positive and meaningful
            if (basis_bp < ENTRY_THRESHOLD_BP) return;

            // Gate 2: basis just spiked this tick (momentum confirmation)
            if (basis_delta < DELTA_TRIGGER_BP) return;

            // Gate 3: perp flow confirming buy pressure
            if (flow_ratio < FLOW_CONFIRM) return;

            // Gate 4: don't enter if vol is dead (no chance of spot following)
            if (vol_ratio < 0.7) return;

            // Enter
            pos_active_    = true;
            entry_price_   = price;
            entry_ts_      = ts;
            entry_basis_   = basis_bp;
            pos_mfe_bp_    = 0.0;
            pos_mae_bp_    = 0.0;
            pos_size_R_    = std::min(1.0, available_R);

            std::printf("[BASIS-ENTRY] %s | basis=%.2fbp (delta=+%.2fbp) | flow=%.2f | vol=%.2f | px=%.4f | size=%.1fR\n",
                symbol_.c_str(), basis_bp, basis_delta, flow_ratio, vol_ratio, price, pos_size_R_);
            std::fflush(stdout);

        } else {
            // ── MANAGE POSITION ────────────────────────────────────────────
            double move_bp  = (price - entry_price_) / entry_price_ * 10000.0;
            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            // Update trailing stop once armed — dynamic distance
            if (move_bp >= TRAIL_ARM_BP) {
                double dist = trail_distance_bp(pos_mfe_bp_);
                trail_stop_bp_ = std::max(trail_stop_bp_, pos_mfe_bp_ - dist);
            }

            bool tp       = move_bp >= TARGET_BP;
            bool sl       = move_bp <= -STOP_BP;
            bool trail    = (pos_mfe_bp_ >= TRAIL_ARM_BP) && (move_bp <= trail_stop_bp_);
            bool timeout  = (ts - entry_ts_) > MAX_HOLD_MS;
            // Early exit: basis collapsed — edge is gone regardless of P&L
            bool basis_collapse = (basis_bp < EXIT_BASIS_BP);
            // Also exit if basis goes negative and we have no profit yet
            bool basis_fading   = (basis_bp < 2.0) && (move_bp < 5.0) && ((ts - entry_ts_) > 3000);

            if (tp || sl || trail || timeout || basis_collapse || basis_fading) {
                double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
                total_pnl_bp_ += net_bp;
                total_trades_++;
                if (net_bp > 0) wins_++;

                const char* reason = trail ? "TRAIL"
                                   : tp    ? "TP"
                                   : sl    ? "SL"
                                   : basis_collapse ? "BASIS_COLLAPSE"
                                   : basis_fading   ? "BASIS_FADING"
                                   : "TIMEOUT";

                std::printf("[BASIS-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | basis_now=%.2fbp | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                    symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP,
                    reason, basis_bp, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
                std::fflush(stdout);

                pos_active_     = false;
                entry_price_    = 0.0;
                trail_stop_bp_  = -9999.0;
                cooldown_until_ms_ = ts + COOLDOWN_MS;
            }
        }
    }

    Stats get_stats() const {
        return {
            pos_active_, entry_price_, pos_mfe_bp_, pos_mae_bp_,
            total_trades_ > 0 ? (double)wins_ / total_trades_ : 0.0,
            total_pnl_bp_, total_trades_,
            prev_basis_bp_, 0.0  // basis_now, delta populated externally in GUI
        };
    }

    bool   pos_active_   = false;
    double pos_size_R_   = 0.0;
    double entry_price_  = 0.0;

private:
    std::string symbol_;

    double  prev_basis_bp_     = 0.0;
    double  entry_basis_       = 0.0;
    double  trail_stop_bp_     = -9999.0; // current trail floor in bp from entry
    double  pos_mfe_bp_        = 0.0;
    double  pos_mae_bp_        = 0.0;
    int64_t entry_ts_          = 0;
    int64_t cooldown_until_ms_ = 0;

    int    wins_          = 0;
    int    total_trades_  = 0;
    double total_pnl_bp_  = 0.0;
};

} // namespace chimera

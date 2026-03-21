#pragma once
// ============================================================================
// OrderbookImbalanceEngine.hpp
// Chimera -- Order Book Imbalance microstructure engine
//
// SIGNAL: When book imbalance exceeds threshold in BUILDUP or BREAKOUT regime,
//         fade the imbalance direction (contrarian — imbalance usually reverts).
//
// DATA USED: tick.book_imbalance, vol_ratio, displacement_bp
// HOLD: 2000ms max, TP=12bp, SL=10bp
// SIZE: 0.5-1.0R, limited by available_R
// COST FLOOR: 12bp (taker round-trip) -- only enter if spread is tight
// ============================================================================

#include <cmath>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace chimera {

class OrderbookImbalanceEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP = 8.0;
    struct Stats {
        bool   active;
        double size_R;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double win_rate;
        double total_pnl_bp;
        int    total_trades;
    };

    explicit OrderbookImbalanceEngine(const std::string& sym) : symbol_(sym) {}
    OrderbookImbalanceEngine() = default;

    // Called every tick from QuadEngine on_tick
    // Imbalance: +1 = all bids, -1 = all asks, 0 = neutral
    // We fade extreme imbalance: high bids -> short, high asks -> long
    void evaluate(
        double   price,
        double   book_imbalance,
        double   spread_bps,
        double   vol_ratio,
        double   perp_basis_bp,  // (perp_mark - spot) / spot * 10000. + = perp premium
        int64_t  ts,
        double   available_R
    ) {
        if (cooldown_ticks_ > 0) { cooldown_ticks_--; return; }

        if (!pos_active_) {

            // Gate: need elevated vol (genuine pressure, not noise)
            if (vol_ratio < 1.15) return;

            // Gate: spread must be tight -- we are taker, cost floor = 12bp
            if (spread_bps > 2.5) return;

            // Gate: extreme imbalance required
            if (std::fabs(book_imbalance) < 0.45) return;

            // Perp confirmation: basis should not be strongly positive (longs already crowded)
            // If perp is at big premium (>8bp), spot long likely already priced in
            if (perp_basis_bp > 8.0) return;

            if (available_R < 0.5) return;

            pos_active_    = true;
            entry_price_   = price;
            pos_size_R_    = std::min(1.0, available_R);
            // Fade: imbalance > 0 (bid heavy) -> SHORT (expect reversion down)
            //       imbalance < 0 (ask heavy) -> LONG  (expect reversion up)
            // Spot only: no short. So only take LONG side (ask heavy -> buy the dip)
            pos_dir_       = (book_imbalance < 0) ? 1 : -1;
            entry_ts_      = ts;
            pos_mfe_bp_    = 0.0;
            pos_mae_bp_    = 0.0;

            if (pos_dir_ == -1) {
                // Short not available in spot — skip this signal
                pos_active_ = false;
                return;
            }

            std::printf("[OBI-ENTRY] %s | imbal=%.2f | spread=%.2fbp | vol=%.2f | basis=%.1fbp | size=%.1fR\n",
                symbol_.c_str(), book_imbalance, spread_bps, vol_ratio, perp_basis_bp, pos_size_R_);
            std::fflush(stdout);
        }
        else {
            double move_bp = (price - entry_price_) / entry_price_ * 10000.0;
            if (pos_dir_ < 0) move_bp = -move_bp;

            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            bool tp      = move_bp >= 12.0;
            bool sl      = move_bp <= -10.0;
            bool timeout = (ts - entry_ts_) > 2000;

            if (tp || sl || timeout) {
                double net_bp = move_bp - ROUND_TRIP_COST_BP;
                total_pnl_bp_ += net_bp * pos_size_R_;
                total_trades_++;
                if (net_bp > 0) wins_++;
                const char* reason = tp ? "TP" : (sl ? "SL" : "TIMEOUT");
                std::printf("[OBI-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | total=%.1fbp\n",
                    symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP, reason, total_pnl_bp_);
                std::fflush(stdout);
                pos_active_     = false;
                cooldown_ticks_ = 40;
            }
        }
    }

    Stats get_stats() const {
        return {
            pos_active_, pos_size_R_, entry_price_, pos_mfe_bp_, pos_mae_bp_,
            total_trades_ > 0 ? (double)wins_ / total_trades_ : 0.0,
            total_pnl_bp_, total_trades_
        };
    }

    bool   pos_active_  = false;
    double pos_size_R_  = 0.0;
    int    cooldown_ticks_ = 0;

private:
    std::string symbol_;
    double  entry_price_ = 0.0;
    int     pos_dir_     = 0;
    int64_t entry_ts_    = 0;
    double  pos_mfe_bp_  = 0.0;
    double  pos_mae_bp_  = 0.0;

    int    wins_          = 0;
    int    total_trades_  = 0;
    double total_pnl_bp_  = 0.0;
};

} // namespace chimera

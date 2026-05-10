#pragma once
// ============================================================================
// OrderbookImbalanceEngine.hpp
// Chimera -- Order Book Imbalance short-term mean-reversion engine
//
// SIGNAL: When book imbalance exceeds threshold in GRIND regime, fade the
//         imbalance direction (contrarian — extreme imbalance usually reverts).
//
// NOT HFT MICROSTRUCTURE: 25bp gross TP / 7bp SL / 2-second hold / 60s cooldown.
//   Slow enough to survive Tokyo→Binance ~25ms latency. The signal source
//   (book imbalance) is freshness-sensitive though, so this engine is the
//   borderline case — backtest before going live.
//
// DATA USED: tick.book_imbalance, tick.spread_bps, vol_ratio, perp_basis_bp
// HOLD: 2000ms max, TP=25bp gross(+10bp net), SL=7bp gross(-22bp net)
// SIZE: 0.5-1.0R, limited by available_R
// COST FLOOR: 15bp (taker round-trip) -- only enter if spread is tight
//
// ── MOVE 2 WRAPPERS (additive, no logic changes) ───────────────────────────
//   shadow_mode flag       : default true; gates any future executor wiring
//   halted_ flag           : set by kill_all(); blocks new entries until reset
//   on_tick(...)           : adapter so main.cpp can call uniformly per tick
//   kill_all()             : flattens any open paper position
//   state_json()           : returns JSON of internal Stats + flags for GUI
//
//   The original evaluate() entry point and its constants are preserved
//   verbatim.
// ============================================================================

#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "core/SymbolIndex.hpp"
#include "risk/Tier1Risk.hpp"

namespace chimera {

class OrderbookImbalanceEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP = 15.0; // 7.5bp/side with BNB discount (0.075% per side)

    // Tier1Risk identity (session 6 wiring)
    static constexpr chimera::risk::EngineType ETYPE =
        chimera::risk::EngineType::OBI;

    // ── MOVE 2: shadow-mode gate (mirrors SwingEngine convention) ────────────
    bool shadow_mode = true;

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

    explicit OrderbookImbalanceEngine(const std::string& sym)
        : symbol_(sym), symbol_id_(sym_id(sym)) {}
    OrderbookImbalanceEngine() = default;

    // Tier1Risk integration setter (session 6 wiring) — null-safe.
    void set_risk(chimera::risk::Tier1Risk* r) { risk_ = r; }

    // Called every tick from QuadEngine on_tick
    // Imbalance: +1 = all bids, -1 = all asks, 0 = neutral
    // We fade extreme imbalance: high bids -> short, high asks -> long
    void evaluate(
        double   price,
        double   book_imbalance,
        double   spread_bps,
        double   vol_ratio,
        double   perp_basis_bp,
        int      regime,         // 0=DEAD,1=GRIND,2=BUILDUP,3=BREAKOUT
        int64_t  ts,
        double   available_R
    ) {
        if (ts < cooldown_until_ms_) return;
        // OBI is mean-reversion: only valid in GRIND (ranging, low vol)
        if (regime != 1) return;  // require GRIND regime

        if (!pos_active_) {

            // Gate: need elevated vol (genuine pressure, not noise)
            if (vol_ratio < 1.25) return;  // raised 1.15->1.25

            // Gate: spread must be tight -- we are taker, cost floor = 12bp
            if (spread_bps > 2.5) return;

            // Gate: extreme imbalance required
            if (std::fabs(book_imbalance) < 0.55) return;  // raised 0.45->0.55: extreme imbalance only

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

            // Tier1Risk: register the open position (only confirmed LONG —
            // the SHORT branch above already exited before reaching here).
            if (risk_) risk_->on_position_open(ETYPE, symbol_id_,
                                               /*is_long=*/true, pos_size_R_);
        }
        else {
            double move_bp = (price - entry_price_) / entry_price_ * 10000.0;
            if (pos_dir_ < 0) move_bp = -move_bp;

            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            bool tp      = move_bp >= 25.0;  // raised 12->25bp: net +17bp after 8bp cost
            bool sl      = move_bp <= -7.0;   // tightened 10->7bp: net -15bp after 8bp cost
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
                cooldown_until_ms_ = ts + 60000;  // 60s cooldown per symbol

                // Tier1Risk: release per-engine R + feed daily-loss circuit.
                // OBI's local net_bp is per-unit; scale by pos_size_R_ for
                // consistency with how total_pnl_bp_ is accumulated above.
                if (risk_) risk_->on_position_close(ETYPE, net_bp * pos_size_R_);
            }
        }
    }

    // ── MOVE 2: uniform per-tick adapter ────────────────────────────────────
    // main.cpp computes regime + vol_ratio externally and passes them in.
    // Until a real per-symbol regime classifier exists, regime is hardcoded
    // to 1 (GRIND) at the call site so OBI's other gates do the work.
    void on_tick(double price,
                 int64_t now_ms,
                 double book_imbalance,
                 double spread_bps,
                 double vol_ratio,
                 double perp_basis_bp,
                 int    regime,
                 double available_R) {
        if (halted_) return;
        if (price <= 0.0) return;
        evaluate(price, book_imbalance, spread_bps, vol_ratio, perp_basis_bp,
                 regime, now_ms, available_R);
    }

    // ── MOVE 2: kill switch (mirrors SwingEngine::kill_all convention) ──────
    void kill_all(double last_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            double exit_px = (last_price > 0.0) ? last_price : entry_price_;
            double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            if (pos_dir_ < 0) move_bp = -move_bp;
            double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            total_trades_++;
            if (net_bp > 0) wins_++;

            std::printf("[OBI-KILL] %s | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_       = false;
            entry_price_      = 0.0;
            cooldown_until_ms_ = (now_ms > 0 ? now_ms : cooldown_until_ms_) + 60000;

            // Tier1Risk: release the per-engine R budget for the killed
            // position. main.cpp's /api/kill handler centralises halt_all().
            if (risk_) risk_->on_position_close(ETYPE, net_bp);
        }
        halted_ = true;
        std::printf("[OBI-KILL] %s | engine halted; clear_halt() to resume\n",
                    symbol_.c_str());
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    Stats get_stats() const {
        return {
            pos_active_, pos_size_R_, entry_price_, pos_mfe_bp_, pos_mae_bp_,
            total_trades_ > 0 ? (double)wins_ / total_trades_ : 0.0,
            total_pnl_bp_, total_trades_
        };
    }

    // ── MOVE 2: per-engine state JSON for GUI / API ─────────────────────────
    std::string state_json(double book_imbalance = 0.0,
                           double spread_bps     = 0.0,
                           double perp_basis_bp  = 0.0,
                           double spot_price     = 0.0) const {
        const Stats s = get_stats();
        const double move_bp = (pos_active_ && entry_price_ > 0.0 && spot_price > 0.0)
            ? ((spot_price - entry_price_) / entry_price_ * 10000.0) * (pos_dir_ < 0 ? -1.0 : 1.0)
            : 0.0;

        std::ostringstream js;
        js << std::fixed << std::setprecision(4);
        js << "{"
           << "\"symbol\":\""        << symbol_              << "\","
           << "\"shadow_mode\":"     << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"          << (halted_     ? "true" : "false") << ","
           << "\"active\":"          << (s.active    ? "true" : "false") << ","
           << "\"entry_price\":"     << s.entry_price        << ","
           << "\"spot_price\":"      << spot_price           << ","
           << "\"move_bp\":"         << move_bp              << ","
           << "\"mfe_bp\":"          << s.mfe_bp             << ","
           << "\"mae_bp\":"          << s.mae_bp             << ","
           << "\"win_rate\":"        << s.win_rate           << ","
           << "\"total_pnl_bp\":"    << s.total_pnl_bp       << ","
           << "\"total_trades\":"    << s.total_trades       << ","
           << "\"book_imbalance\":"  << book_imbalance       << ","
           << "\"spread_bps\":"      << spread_bps           << ","
           << "\"perp_basis_bp\":"   << perp_basis_bp        << ","
           << "\"size_R\":"          << pos_size_R_
           << "}";
        return js.str();
    }

    bool   pos_active_  = false;
    double pos_size_R_  = 0.0;
    int64_t cooldown_until_ms_ = 0;  // time-based cooldown

private:
    // ── MOVE 2: kill-switch state ────────────────────────────────────────────
    bool    halted_      = false;

    std::string symbol_;

    // ── Tier1Risk wiring (session 6) ─────────────────────────────────────────
    // Declared after symbol_ so the constructor initializer list
    // `: symbol_(sym), symbol_id_(sym_id(sym))` matches declaration order
    // (silences -Wreorder).
    chimera::risk::Tier1Risk* risk_      = nullptr;
    int                       symbol_id_ = -1;

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

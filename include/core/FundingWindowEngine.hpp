#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// FundingWindowEngine.hpp
// Chimera — Pre-Funding Window Trading Engine
//
// EDGE:
//   Funding payments occur every 8h: 00:00, 08:00, 16:00 UTC.
//   In the 2-4 minutes before each payment, perp traders with losing funding
//   positions close/reduce them. This creates predictable directional flow:
//
//   Positive funding (longs paying shorts):
//     -> Long holders close before paying -> spot price dips briefly
//     -> After payment, buying resumes -> LONG spot in window
//
//   Negative funding (shorts paying longs):
//     -> Short holders close before paying -> spot price pumps briefly
//     -> LONG spot in window (can't short spot)
//
//   The key insight: |basis| widens before funding, then snaps back after.
//   Trading the snapback is the signal.
//
// ENTRY CONDITIONS (all must be true):
//   1. Within WINDOW_SECS (120s) of funding time
//   2. |funding_rate| >= RATE_THRESHOLD (>=15bp/8h — meaningful imbalance)
//   3. |basis| >= BASIS_THRESHOLD (>=4bp — perp dislocated from spot)
//   4. For LONG: basis < 0 (perp below spot) OR positive funding snapping
//   5. Not in cooldown (4h between entries per symbol)
//
// EXIT:
//   Dynamic trail — arms at 30bp, trails 15bp below peak
//   Hard SL at -20bp
//   Hard timeout at 8 minutes (funding usually resolves in <5min)
//
// COST MODEL:
//   Round-trip: 15bp (BNB discount, maker entry)
//   Net TP at 80bp gross: +65bp
//   Break-even WR: ~30% (edge is structural, not just momentum)
//
// SYMBOLS: BTC and ETH only — most reliable funding signals
//
// ── MOVE 2 WRAPPERS (additive, no logic changes) ───────────────────────────
//   shadow_mode flag       : default true; gates any future executor wiring
//   halted_ flag           : set by kill_all(); blocks new entries until reset
//   on_tick(...)           : adapter so main.cpp can call uniformly per tick;
//                            delegates to evaluate() unchanged
//   kill_all()             : flattens any open paper position, books P&L,
//                            sets halted_ = true
//   state_json()           : returns JSON of internal Stats + flags for GUI
//
//   The original evaluate() entry point and its constants are preserved
//   verbatim so that the canonical entry/exit logic remains the same one
//   that will later be exercised by the backtest harness (Move 2 task C).
// ============================================================================

#include <cmath>
#include <ctime>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace chimera {

class FundingWindowEngine {
public:
    static constexpr double   ROUND_TRIP_COST_BP  = TradingConfig::MAKER_ROUND_TRIP_BP; // 15bp
    static constexpr double   RATE_THRESHOLD      = 0.00015; // 15bp/8h minimum
    static constexpr double   BASIS_THRESHOLD     = 3.0;     // 3bp basis dislocation
    static constexpr int      WINDOW_SECS         = 180;     // 3 min before funding
    static constexpr double   STOP_BP             = 20.0;
    static constexpr double   TRAIL_ARM_BP        = 30.0;
    static constexpr int64_t  MAX_HOLD_MS         = 480000;  // 8 min
    static constexpr int64_t  COOLDOWN_MS         = 14400000;// 4h — one per funding session

    explicit FundingWindowEngine(const std::string& sym = "") : symbol_(sym) {}

    // ── MOVE 2: shadow-mode gate (mirrors SwingEngine convention) ────────────
    // Default true so the engine paper-trades. Real-execution wiring is
    // intentionally deferred until a PerpExecutor exists (Move 2.5).
    bool shadow_mode = true;

    // Called every tick from QuadEngine
    // funding_rate: from PerpFeed::funding_rate(id) — real-time, updates every 3s
    // basis_bp:     from PerpFeed::basis_bp(id, spot) — (perp - spot) / spot * 10000
    void evaluate(
        double  price,
        double  funding_rate,  // fractional (e.g. 0.0001 = 1bp/8h)
        double  basis_bp,      // positive = perp premium, negative = perp discount
        int64_t ts,
        double  available_R
    ) {
        if (ts < cooldown_until_ms_) return;
        if (available_R < 0.5) return;

        int secs_to_funding = seconds_to_funding();

        if (!pos_active_) {
            // ── ENTRY GATE ────────────────────────────────────────────────
            // Must be within funding window
            if (secs_to_funding > WINDOW_SECS) return;
            if (secs_to_funding < 0) return; // just passed, wait for next

            // Funding rate must be meaningful
            double rate_abs = std::fabs(funding_rate);
            if (rate_abs < RATE_THRESHOLD) return;

            // Basis must show dislocation
            double basis_abs = std::fabs(basis_bp);
            if (basis_abs < BASIS_THRESHOLD) return;

            // Determine direction:
            // Positive funding + perp at premium = longs will close = short signal (skip, spot only)
            // Positive funding + perp at discount = basis snapping back = LONG
            // Negative funding + perp at discount = shorts closing = LONG (spot follows perp up)
            // Negative funding + perp at premium = skip
            bool do_long = false;

            if (funding_rate > 0 && basis_bp < -BASIS_THRESHOLD) {
                // Longs paid but perp below spot = unusual, basis will snap back up = LONG
                do_long = true;
            } else if (funding_rate < 0 && basis_bp < 0) {
                // Shorts paying, perp below spot = shorts closing = spot follows up = LONG
                do_long = true;
            } else if (funding_rate < 0 && basis_bp > BASIS_THRESHOLD) {
                // Shorts paying, perp at premium = forced short covering will pump = LONG
                do_long = true;
            }

            if (!do_long) return;

            // Enter
            pos_active_    = true;
            entry_price_   = price;
            entry_ts_      = ts;
            entry_rate_    = funding_rate;
            entry_basis_   = basis_bp;
            entry_secs_    = secs_to_funding;
            pos_mfe_bp_    = 0.0;
            pos_mae_bp_    = 0.0;
            trail_floor_bp_= -9999.0;
            pos_size_R_    = std::min(1.0, available_R);

            std::printf("[FUND-WIN-ENTRY] %s | rate=%.4f%% (%.1fbp/8h) | basis=%.2fbp | "
                        "%ds to funding | px=%.4f | size=%.1fR\n",
                symbol_.c_str(),
                funding_rate * 100.0,
                funding_rate * 10000.0,
                basis_bp,
                secs_to_funding,
                price,
                pos_size_R_);
            std::fflush(stdout);

        } else {
            // ── MANAGE POSITION ────────────────────────────────────────────
            double move_bp = (price - entry_price_) / entry_price_ * 10000.0;
            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            // Dynamic trailing stop
            if (pos_mfe_bp_ >= TRAIL_ARM_BP) {
                double dist = pos_mfe_bp_ < 50  ? 20.0
                            : pos_mfe_bp_ < 100 ? 18.0
                            : pos_mfe_bp_ < 200 ? 15.0
                            : pos_mfe_bp_ < 300 ? 12.0 : 8.0;
                trail_floor_bp_ = std::max(trail_floor_bp_, pos_mfe_bp_ - dist);
            }

            bool sl      = move_bp <= -STOP_BP;
            bool trail   = (pos_mfe_bp_ >= TRAIL_ARM_BP) && (move_bp <= trail_floor_bp_);
            bool timeout = (ts - entry_ts_) > MAX_HOLD_MS;

            if (sl || trail || timeout) {
                double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
                total_pnl_bp_ += net_bp;
                total_trades_++;
                if (net_bp > 0) wins_++;

                const char* reason = trail ? "TRAIL" : sl ? "SL" : "TIMEOUT";
                std::printf("[FUND-WIN-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | "
                            "reason=%s | mfe=%.1f | total=%.1fbp\n",
                    symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP,
                    reason, pos_mfe_bp_, total_pnl_bp_);
                std::fflush(stdout);

                pos_active_      = false;
                entry_price_     = 0.0;
                trail_floor_bp_  = -9999.0;
                cooldown_until_ms_ = ts + COOLDOWN_MS;
            }
        }
    }

    // ── MOVE 2: uniform per-tick adapter ────────────────────────────────────
    // main.cpp resolves funding_rate / basis_bp / available_R from PerpFeed and
    // a (placeholder) risk budget, then calls this. We keep evaluate() as the
    // canonical implementation so the future backtest harness can replay it
    // directly with synthetic times.
    void on_tick(double price,
                 int64_t now_ms,
                 double funding_rate,
                 double basis_bp,
                 double available_R) {
        if (halted_) return;
        if (price <= 0.0) return;
        // shadow_mode currently has no behavioural difference because there is
        // no executor wired — every trade is paper. Once a PerpExecutor lands,
        // shadow_mode = false will route entries/exits through it instead of
        // (or in addition to) the printf paper log.
        evaluate(price, funding_rate, basis_bp, now_ms, available_R);
    }

    // ── MOVE 2: kill switch (mirrors SwingEngine::kill_all convention) ──────
    // Flattens any open paper position with proper P&L accounting using the
    // last seen price (passed in by caller — the engine doesn't store it on
    // its own outside an open position). Sets halted_ so no new entries fire
    // until clear_halt() is called.
    void kill_all(double last_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            double exit_px = (last_price > 0.0) ? last_price : entry_price_;
            double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            total_trades_++;
            if (net_bp > 0) wins_++;

            std::printf("[FUND-WIN-KILL] %s | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f | total=%.1fbp\n",
                symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_       = false;
            entry_price_      = 0.0;
            trail_floor_bp_   = -9999.0;
            cooldown_until_ms_ = (now_ms > 0 ? now_ms : cooldown_until_ms_) + COOLDOWN_MS;
        }
        halted_ = true;
        std::printf("[FUND-WIN-KILL] %s | engine halted; clear_halt() to resume\n",
                    symbol_.c_str());
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }

    bool is_halted() const { return halted_; }

    // ── MOVE 2 backtest hook: inject a virtual wall clock ─────────────────
    // Backtest harness calls this before each evaluate() to set the simulated
    // current time. Live code never calls this, so backtest_time_override_seconds_
    // stays 0 and seconds_to_funding() uses std::time(nullptr) as before.
    void set_backtest_time(int64_t epoch_seconds) {
        backtest_time_override_seconds_ = epoch_seconds;
    }

    struct Stats {
        bool   active;
        double entry_price;
        double mfe_bp;
        double win_rate;
        double total_pnl_bp;
        int    total_trades;
        int    secs_to_next_funding;
        double current_rate;
        double current_basis;
        double trail_floor_bp;
        bool   trail_armed;
    };

    Stats get_stats(double funding_rate = 0.0, double basis_bp = 0.0) const {
        return {
            pos_active_, entry_price_, pos_mfe_bp_,
            total_trades_ > 0 ? (double)wins_ / total_trades_ : 0.0,
            total_pnl_bp_, total_trades_,
            seconds_to_funding(),
            funding_rate, basis_bp,
            trail_floor_bp_,
            pos_mfe_bp_ >= TRAIL_ARM_BP
        };
    }

    // ── MOVE 2: per-engine state JSON for GUI / API ─────────────────────────
    // Caller passes the latest funding_rate / basis_bp / spot_price so the
    // snapshot reflects current market context, not just internal counters.
    std::string state_json(double funding_rate = 0.0,
                           double basis_bp     = 0.0,
                           double spot_price   = 0.0) const {
        const Stats s = get_stats(funding_rate, basis_bp);
        const double move_bp = (pos_active_ && entry_price_ > 0.0 && spot_price > 0.0)
            ? (spot_price - entry_price_) / entry_price_ * 10000.0
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
           << "\"trail_floor_bp\":"  << s.trail_floor_bp     << ","
           << "\"trail_armed\":"     << (s.trail_armed ? "true" : "false") << ","
           << "\"win_rate\":"        << s.win_rate           << ","
           << "\"total_pnl_bp\":"    << s.total_pnl_bp       << ","
           << "\"total_trades\":"    << s.total_trades       << ","
           << "\"secs_to_next_funding\":" << s.secs_to_next_funding << ","
           << "\"current_rate\":"    << s.current_rate       << ","
           << "\"current_basis\":"   << s.current_basis      << ","
           << "\"size_R\":"          << pos_size_R_
           << "}";
        return js.str();
    }

    bool   pos_active_  = false;
    double pos_size_R_  = 0.0;
    double entry_price_ = 0.0;

private:
    std::string symbol_;

    // ── MOVE 2: kill-switch state ────────────────────────────────────────────
    bool    halted_           = false;

    double  pos_mfe_bp_       = 0.0;
    double  pos_mae_bp_       = 0.0;
    double  trail_floor_bp_   = -9999.0;
    int64_t entry_ts_         = 0;
    double  entry_rate_       = 0.0;
    double  entry_basis_      = 0.0;
    int     entry_secs_       = 0;
    int64_t cooldown_until_ms_= 0;

    int    wins_          = 0;
    int    total_trades_  = 0;
    double total_pnl_bp_  = 0.0;

    // Returns seconds until next funding payment (00:00, 08:00, 16:00 UTC).
    // Live: reads std::time(nullptr). Backtest: reads backtest_time_override_seconds_
    // when set via set_backtest_time(). The override is the ONLY behavioural
    // difference vs the original — when override is 0 (default), this method is
    // bit-for-bit identical to the live path.
    int seconds_to_funding() const {
        std::time_t now = (backtest_time_override_seconds_ > 0)
            ? (std::time_t)backtest_time_override_seconds_
            : std::time(nullptr);
        std::tm* g = std::gmtime(&now);
        int now_s = g->tm_hour * 3600 + g->tm_min * 60 + g->tm_sec;

        // Next funding boundary
        int next_boundary;
        if      (now_s <  8 * 3600) next_boundary =  8 * 3600;
        else if (now_s < 16 * 3600) next_boundary = 16 * 3600;
        else                         next_boundary = 24 * 3600;

        return next_boundary - now_s;
    }

    int64_t backtest_time_override_seconds_ = 0;  // 0 = use real wall clock
};

} // namespace chimera

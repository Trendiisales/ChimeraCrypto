#pragma once
// ============================================================================
// LiqBracketEngine.hpp
// Chimera -- Liquidation-Confirmed Bracket Engine
//
// WHAT IT DOES:
//   Detects price compression (tight range, low vol), waits for a breakout
//   confirmed by BOTH a liquidation spike AND perp leading spot.
//   Once confirmed, arms a bracket: TP=70bp, SL=28bp from entry.
//
// WHY THREE GATES:
//   1. Range compression    -> confirms market is coiled (not just choppy)
//   2. Liquidation spike    -> confirms forced orders driving the move
//   3. Perp leading spot    -> confirms the move is real, not a fake wick
//
// COST MODEL (taker entry + taker exit):
//   Round-trip cost: 15bp (7.5bp/side with BNB discount = TradingConfig::MAKER_ROUND_TRIP_BP)
//   net TP:  80 - 15 = +65bp
//   net SL: -28 - 15 = -43bp
//   Break-even WR: 43 / (65+43) = 39.8% -- positive EV above 40% WR
//
// REGIME GATE:
//   Only fires when COMPRESSION regime (low vol, tight range).
//   QuadEngine passes ms.regime; engine returns immediately if regime >= 2.
//   Prevents bracket firing into an already-trending market.
//
// PARAMETERS (all calibrated for 15bp round-trip cost, MAKER_ROUND_TRIP_BP):
//   MIN_RANGE_BP     = 20bp  (range must be meaningful)
//   MAX_RANGE_BP     = 60bp  (range must not be a trend already)
//   MIN_RANGE_TICKS  = 50    (~5s of data at typical tick rate)
//   BREAKOUT_BP      = 12bp  (breakout must clear range edge)
//   ENTRY_BUFFER_BP  = 3bp   (enter slightly beyond breakout)
//   STOP_BP          = 28bp  (tight enough to limit loss, wide enough for noise)
//   TARGET_BP        = unlimited trail  (trail exits; must clear 15bp cost)
//   LIQ_THRESHOLD    = $150k (meaningful liquidation, not noise)
//   PERP_LEAD_BP     = 5bp   (perp must be leading spot clearly)
//   COOLDOWN_MS      = 120s  (prevents re-entry after exit)
//
// INTEGRATION:
//   One instance per symbol in QuadEngineBalancedEngine.
//   evaluate() called each tick in COMPRESSION regime only.
//   When armed(), QuadEngine logs the trade via push_trade on exit.
// ============================================================================

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>
#include <cstdio>

namespace chimera {

class LiqBracketEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP = TradingConfig::MAKER_ROUND_TRIP_BP; // 15bp

    // Parameters
    static constexpr double MIN_RANGE_BP     = 20.0;
    static constexpr double MAX_RANGE_BP     = 60.0;
    static constexpr int    MIN_RANGE_TICKS  = 200;  // ~20s of compression needed for meaningful bracket
    static constexpr double BREAKOUT_BP      = 12.0;
    static constexpr double ENTRY_BUFFER_BP  = 3.0;
    static constexpr double STOP_BP          = 28.0;
    static constexpr double TARGET_BP        = 2000.0; // hard cap — effectively unlimited, trail always exits first
    static constexpr double TRAIL_ARM_BP     = 40.0;   // start trailing once +40bp profit

    // Dynamic trail distance — tighter as move gets larger to capture more of big runs
    static double trail_distance_bp(double peak_bp) {
        if (peak_bp < 50.0)  return 20.0;
        if (peak_bp < 100.0) return 18.0;
        if (peak_bp < 200.0) return 15.0;
        if (peak_bp < 300.0) return 12.0;
        return 8.0;  // >= 300bp: 8bp trail captures 97%+
    }
    static constexpr double LIQ_THRESHOLD    = 150000.0;  // $150k notional — real cascade events only
    static constexpr double PERP_LEAD_BP     = 5.0;
    static constexpr int64_t COOLDOWN_MS     = 120000;    // 2 min per symbol

    enum class State { IDLE, RANGE_BUILD, WAIT_CONFIRM, ARMED, IN_POSITION, COOLDOWN };

    // Position state (matches pattern of StructuralEngine/ConvexShockEngine)
    struct BracketPos {
        bool   active      = false;
        bool   long_side   = false;
        double entry_price = 0.0;
        double stop_price  = 0.0;
        double target_price= 0.0;
        double size_R      = 0.0;
        double mfe_bp      = 0.0;
        double mae_bp      = 0.0;
        void reset() {
            active = long_side = false;
            entry_price = stop_price = target_price = size_R = mfe_bp = mae_bp = 0.0;
        }
    };

    struct Stats {
        bool   active;
        double size_R;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double win_rate;
        double total_pnl_bp;
        int    total_trades;
        State  state;
        double range_pct;    // 0-1 how built the range is
    };

    BracketPos pos;
    State state = State::IDLE;

    explicit LiqBracketEngine(const std::string& sym = "") : symbol_(sym) {}

    // ── Main evaluate — called each tick from QuadEngine ──────────────────
    // regime: 0=DEAD,1=GRIND,2=BUILDUP,3=BREAKOUT
    // liq_notional: current pending liquidation notional from LiquidationEngine
    // perp_basis_bp: (perp_mark - spot) / spot * 10000
    void evaluate(
        double   price,
        double   vol_ratio,
        double   liq_notional,   // from LiquidationEngine::get_notional(id)
        double   perp_basis_bp,  // from PerpFeed::basis_bp(id, price)
        int      regime,
        int64_t  ts,
        double   available_R
    ) {
        // REGIME GATE: block only in BREAKOUT (regime 3) — don't chase breakouts
        // GRIND and BUILDUP are valid for range-building and confirmation
        if (regime >= 3) {
            if (!pos.active) return;  // allow managing active position
        }

        if (ts < cooldown_until_ms_) return;
        if (available_R < 0.5) return;

        update_range(price);

        switch (state) {

        case State::IDLE:
            range_high_ = price;
            range_low_  = price;
            range_ticks_ = 0;
            state = State::RANGE_BUILD;
            break;

        case State::RANGE_BUILD:
            range_ticks_++;
            range_high_ = std::max(range_high_, price);
            range_low_  = std::min(range_low_,  price);

            if (range_ticks_ < MIN_RANGE_TICKS) return;

            {
                double range_bp = (range_high_ - range_low_) / price * 10000.0;
                if (range_bp < MIN_RANGE_BP) return;
                if (range_bp > MAX_RANGE_BP) { state = State::IDLE; return; }
                // Also require vol is genuinely compressed
                if (vol_ratio >= 1.2) return;   // range builds unless vol actively expanding
                state = State::WAIT_CONFIRM;
                std::printf("[BRACKET] %s range built: %.1fbp over %d ticks | h=%.2f l=%.2f\n",
                    symbol_.c_str(), range_bp, range_ticks_, range_high_, range_low_);
                std::fflush(stdout);
            }
            break;

        case State::WAIT_CONFIRM: {
            double up_move   = (price - range_high_) / price * 10000.0;
            double down_move = (range_low_ - price)  / price * 10000.0;

            // LONG breakout: price above range + short liquidation + perp premium
            if (up_move > BREAKOUT_BP
                && liq_notional >= LIQ_THRESHOLD
                && perp_basis_bp > PERP_LEAD_BP)
            {
                arm(price, true, available_R, ts);
                return;
            }

            // SHORT breakout: spot only -- skip (no shorting in spot)
            // If range breaks down without confirmation, reset
            if (down_move > BREAKOUT_BP && liq_notional < LIQ_THRESHOLD) {
                // Fake breakdown with no liquidation — reset range
                std::printf("[BRACKET] %s false breakdown (no liq) — resetting\n", symbol_.c_str());
                std::fflush(stdout);
                state = State::IDLE;
            }
            break;
        }

        case State::ARMED:
            // Position not yet filled — manage via manage() externally
            // In this sim implementation, fill immediately at arm price
            if (!pos.active) {
                pos.active      = true;
                pos.entry_price = pos.entry_price;  // already set in arm()
                state = State::IN_POSITION;
            }
            break;

        case State::IN_POSITION:
            manage(price, ts);
            break;

        case State::COOLDOWN:
            break;
        }
    }

    // manage() called every tick when in position — trailing stop
    bool manage(double price, int64_t ts) {
        if (!pos.active) return false;

        double move_bp = (price - pos.entry_price) / pos.entry_price * 10000.0;
        pos.mfe_bp = std::max(pos.mfe_bp, move_bp);
        pos.mae_bp = std::min(pos.mae_bp, move_bp);

        // Update trailing stop once armed — tighter as move gets bigger
        if (pos.mfe_bp >= TRAIL_ARM_BP) {
            double dist       = trail_distance_bp(pos.mfe_bp);
            double trail_floor = pos.mfe_bp - dist;
            pos.stop_price = std::max(pos.stop_price,
                pos.entry_price * (1.0 + trail_floor / 10000.0));
        }

        bool tp = move_bp >= TARGET_BP;           // hard cap
        bool sl = price <= pos.stop_price;        // initial SL or trail floor

        if (tp || sl) {
            double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos.size_R;
            total_pnl_bp_ += net_bp;
            total_trades_++;
            if (net_bp > 0) wins_++;
            const char* reason = (move_bp >= TRAIL_ARM_BP) ? "TRAIL" : (tp ? "TP" : "SL");
            std::printf("[BRACKET-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | mfe=%.1f | total=%.1fbp\n",
                symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP, reason, pos.mfe_bp, total_pnl_bp_);
            std::fflush(stdout);
            exit_trade(ts);
            return true;
        }
        return false;
    }

    Stats get_stats() const {
        double range_pct = 0.0;
        if (state == State::RANGE_BUILD && range_ticks_ > 0) {
            range_pct = std::min(1.0, (double)range_ticks_ / MIN_RANGE_TICKS);
        } else if (state == State::WAIT_CONFIRM) {
            range_pct = 1.0;
        }
        return {
            pos.active, pos.size_R, pos.entry_price, pos.mfe_bp, pos.mae_bp,
            total_trades_ > 0 ? (double)wins_ / total_trades_ : 0.0,
            total_pnl_bp_, total_trades_, state, range_pct
        };
    }

    int cooldown_ticks = 0;  // legacy compat — unused (time-based now)

private:
    std::string symbol_;
    double range_high_  = 0.0;
    double range_low_   = 0.0;
    int    range_ticks_ = 0;
    int64_t cooldown_until_ms_ = 0;

    int    wins_         = 0;
    int    total_trades_ = 0;
    double total_pnl_bp_ = 0.0;

    // Rolling range tracker for range_pct display
    void update_range(double price) {
        if (state == State::RANGE_BUILD || state == State::IDLE) {
            range_high_ = std::max(range_high_, price);
            if (range_low_ > 0) range_low_ = std::min(range_low_, price);
            else range_low_ = price;
        }
    }

    void arm(double price, bool long_side, double available_R, int64_t /*ts*/) {
        state = State::ARMED;
        pos.long_side   = long_side;
        pos.size_R      = std::min(1.0, available_R);
        // Entry slightly beyond breakout to confirm
        pos.entry_price = long_side
            ? price * (1.0 + ENTRY_BUFFER_BP / 10000.0)
            : price * (1.0 - ENTRY_BUFFER_BP / 10000.0);
        pos.stop_price  = long_side
            ? pos.entry_price * (1.0 - STOP_BP   / 10000.0)
            : pos.entry_price * (1.0 + STOP_BP   / 10000.0);
        pos.target_price = long_side
            ? pos.entry_price * (1.0 + TARGET_BP / 10000.0)
            : pos.entry_price * (1.0 - TARGET_BP / 10000.0);
        pos.mfe_bp = pos.mae_bp = 0.0;
        pos.active = false;  // will be set active in next ARMED tick

        std::printf("[BRACKET-ARM] %s | %s | entry=%.2f stop=%.2f target=%.2f | liq+perp confirmed\n",
            symbol_.c_str(), long_side ? "LONG" : "SHORT",
            pos.entry_price, pos.stop_price, pos.target_price);
        std::fflush(stdout);
    }

    void exit_trade(int64_t ts) {
        pos.reset();
        state = State::COOLDOWN;
        cooldown_until_ms_ = ts + COOLDOWN_MS;
        // Reset for next range build
        range_high_ = 0.0;
        range_low_  = 0.0;
        range_ticks_ = 0;
    }
};

} // namespace chimera

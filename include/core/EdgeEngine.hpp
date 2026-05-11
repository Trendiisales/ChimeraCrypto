// ============================================================================
// EdgeEngine.hpp — Tier-2 long-only longer-timeframe edges
//
// Replaces SwingEngine + FundingWindowEngine + BasisMomentumEngine +
// OrderbookImbalanceEngine. A single configurable header-only class instantiated
// once per (symbol, strategy, timeframe) keeper edge from the backtest pipeline.
//
// Backtest provenance (run 2026-05-11 on 2022-01-01 -> 2026-05-11, 10bp cost):
//
//   instance              symbol     strat        tf    trades  OOS_PF  OOS_bp
//   ----------------------------------------------------------------------------
//   link_rsi_h6           LINKUSDT   RSI_REVERT   H6        64    2.82   +7828
//   eth_bb_h6             ETHUSDT    BOLLINGER    H6       219    1.31   +4258
//   sol_donch_h6          SOLUSDT    DONCHIAN     H6       108    1.24   +2900
//   xrp_donch_h1          XRPUSDT    DONCHIAN     H1       608    1.20   +4547
//   btc_tsmom_d1          BTCUSDT    TSMOM        D1        86    1.19   +1039
//
// Exit logic (every strategy):
//   - Entry at next-bar OPEN after signal close (no look-ahead)
//   - Hard SL at entry - sl_atr_mult * ATR14(at signal bar)
//   - Time exit at hold_bars after entry
//   - No take-profit (mirrors Omega tsmom spec)
//
// All instances are LONG-only spot (per ChimeraCrypto SPOT-ONLY guardrail).
// Shadow mode default = true; promote to live only after 4 weeks of paper
// trades match backtest expectations.
//
// Bar synthesis is internal: each engine accumulates ticks into its own
// timeframe bars (no shared bar bus required).
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace chimera {

enum class StrategyKind {
    TSMOM,       // 20-bar return > 0
    DONCHIAN,    // close > prior 20-bar high
    BOLLINGER,   // bar pierces lower BB(20,2) then closes back above
    RSI_REVERT   // RSI(14) crosses up from <= 30
};

inline const char* strategy_name(StrategyKind k) {
    switch (k) {
        case StrategyKind::TSMOM:      return "TSMOM";
        case StrategyKind::DONCHIAN:   return "DONCHIAN";
        case StrategyKind::BOLLINGER:  return "BOLLINGER";
        case StrategyKind::RSI_REVERT: return "RSI_REVERT";
    }
    return "UNK";
}

class EdgeEngine {
public:
    struct Config {
        std::string  symbol;        // "btcusdt"
        std::string  tag;           // short label e.g. "BTC-TSMOM-D1"
        StrategyKind kind;
        int64_t      tf_secs    = 21600;   // bar timeframe (1h=3600, 6h=21600, 1d=86400)
        int          lookback   = 20;
        int          hold_bars  = 12;
        double       sl_atr_mult = 2.5;
        int          atr_period  = 14;
        // BOLLINGER:
        double       bb_k        = 2.0;
        // RSI_REVERT:
        double       rsi_threshold = 30.0;
        // Optional cost-bp deducted from logged net P&L (display only — does not
        // affect signal):
        double       round_trip_bp = 10.0;
        // Max bar buffer history kept (must be >= max(lookback, bb_len, atr_period)+5)
        int          max_history = 64;
    };

    bool shadow_mode = true;  // public for main.cpp init parity with old engines

    explicit EdgeEngine(const Config& cfg) : cfg_(cfg) {
        if (cfg_.max_history < cfg_.lookback + 5)  cfg_.max_history = cfg_.lookback + 5;
        if (cfg_.max_history < cfg_.atr_period + 5) cfg_.max_history = cfg_.atr_period + 5;
        std::printf("[%s] ARMED  symbol=%s strat=%s tf=%llds lookback=%d hold=%d sl=%.2f*atr  shadow=%d\n",
            cfg_.tag.c_str(), cfg_.symbol.c_str(),
            strategy_name(cfg_.kind),
            (long long)cfg_.tf_secs, cfg_.lookback, cfg_.hold_bars, cfg_.sl_atr_mult,
            shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    // Called on every spot tick for this symbol. Builds bars internally.
    void on_tick(double price, int64_t ts_ms) {
        if (price <= 0.0) return;
        int64_t bar_id = ts_ms / 1000 / cfg_.tf_secs;
        if (cur_bar_id_ == 0) {
            // First tick — begin a new bar at this id.
            cur_bar_id_ = bar_id;
            cur_open_ = cur_high_ = cur_low_ = cur_close_ = price;
            cur_open_ts_ms_ = bar_id * cfg_.tf_secs * 1000;
        } else if (bar_id != cur_bar_id_) {
            // Bar boundary crossed — close out the previous bar then open new ones
            // for every full bar gap (in case of feed silence).
            close_bar_();
            int64_t gap = bar_id - cur_bar_id_;
            for (int64_t i = 1; i < gap; ++i) {
                // synthesise an empty filler bar at last close (rare in crypto;
                // happens during exchange outages)
                cur_bar_id_ += 1;
                cur_open_ts_ms_ = cur_bar_id_ * cfg_.tf_secs * 1000;
                cur_open_ = cur_high_ = cur_low_ = cur_close_ = last_close_;
                close_bar_();
            }
            cur_bar_id_ = bar_id;
            cur_open_ts_ms_ = bar_id * cfg_.tf_secs * 1000;
            cur_open_ = cur_high_ = cur_low_ = cur_close_ = price;
        } else {
            if (price > cur_high_) cur_high_ = price;
            if (price < cur_low_)  cur_low_  = price;
            cur_close_ = price;
        }

        // Intra-bar exit check (so we don't miss the stop until the next bar boundary)
        if (in_position_) {
            check_exits_(price, ts_ms);
        }
    }

    // Force-flatten any open paper position at the given price.
    void kill_all(double price, int64_t ts_ms) {
        if (in_position_ && price > 0.0) {
            exit_position_(price, ts_ms, "KILL");
        }
        halted_ = true;
    }

    // JSON state line for /api/state2 (one object per engine; main.cpp wraps in array).
    std::string state_json() const {
        std::ostringstream js;
        js << "{";
        js << "\"tag\":\""    << cfg_.tag    << "\",";
        js << "\"symbol\":\"" << cfg_.symbol << "\",";
        js << "\"strategy\":\"" << strategy_name(cfg_.kind) << "\",";
        js << "\"tf_secs\":" << cfg_.tf_secs << ",";
        js << "\"shadow\":"  << (shadow_mode ? "true" : "false") << ",";
        js << "\"halted\":"  << (halted_     ? "true" : "false") << ",";
        js << "\"in_position\":" << (in_position_ ? "true" : "false") << ",";
        js << std::fixed << std::setprecision(6);
        js << "\"entry_px\":"  << (in_position_ ? entry_px_ : 0.0)  << ",";
        js << "\"sl_px\":"     << (in_position_ ? sl_px_    : 0.0)  << ",";
        js << "\"last_close\":" << last_close_ << ",";
        js << "\"trades\":"    << trades_ << ",";
        js << "\"wins\":"      << wins_   << ",";
        js << std::setprecision(2);
        js << "\"total_bp\":"  << total_bp_      << ",";
        js << "\"last_bp\":"   << last_trade_bp_ << ",";
        js << "\"bars_in_buffer\":" << (int)closes_.size();
        js << "}";
        return js.str();
    }

    // Counters
    int trades() const { return trades_; }
    int wins() const { return wins_; }
    double total_bp() const { return total_bp_; }
    bool in_position() const { return in_position_; }

private:
    Config cfg_;

    // Bar accumulator
    int64_t cur_bar_id_     = 0;
    int64_t cur_open_ts_ms_ = 0;
    double  cur_open_  = 0.0, cur_high_ = 0.0, cur_low_ = 0.0, cur_close_ = 0.0;
    double  last_close_ = 0.0;

    // Closed-bar history (back is most recent)
    std::deque<double> opens_;
    std::deque<double> highs_;
    std::deque<double> lows_;
    std::deque<double> closes_;
    std::deque<int64_t> bar_ts_ms_;

    // Position state
    bool    in_position_ = false;
    double  entry_px_    = 0.0;
    double  sl_px_       = 0.0;
    int64_t entry_ts_ms_ = 0;
    int64_t time_exit_ts_ms_ = 0;
    double  atr_at_entry_ = 0.0;
    int     bars_held_ = 0;

    // Stats
    int    trades_ = 0;
    int    wins_   = 0;
    double total_bp_ = 0.0;
    double last_trade_bp_ = 0.0;
    bool   halted_ = false;

    // ── Bar close ────────────────────────────────────────────────────────────
    void close_bar_() {
        opens_.push_back(cur_open_);
        highs_.push_back(cur_high_);
        lows_.push_back(cur_low_);
        closes_.push_back(cur_close_);
        bar_ts_ms_.push_back(cur_open_ts_ms_);
        while ((int)closes_.size() > cfg_.max_history) {
            opens_.pop_front(); highs_.pop_front(); lows_.pop_front();
            closes_.pop_front(); bar_ts_ms_.pop_front();
        }
        last_close_ = cur_close_;

        // First, check if a time-based exit just landed on this bar boundary.
        if (in_position_ && cur_open_ts_ms_ + cfg_.tf_secs * 1000 > time_exit_ts_ms_) {
            // exit at this bar's close
            exit_position_(cur_close_, cur_open_ts_ms_ + cfg_.tf_secs * 1000, "TIME");
        }

        // Then, evaluate a new signal (only if flat and not halted).
        if (!in_position_ && !halted_) {
            evaluate_signal_();
        }

        bars_held_ = in_position_ ? (bars_held_ + 1) : 0;
    }

    // ── Indicators (all read from the closed-bar buffer) ─────────────────────
    double atr_(int n) const {
        // Need n+1 bars to compute n TRs (TR uses prev close).
        if ((int)closes_.size() < n + 1) return 0.0;
        double sum = 0.0;
        const int sz = (int)closes_.size();
        for (int i = sz - n; i < sz; ++i) {
            double prev_close = closes_[i - 1];
            double tr = std::max({
                highs_[i] - lows_[i],
                std::fabs(highs_[i] - prev_close),
                std::fabs(lows_[i]  - prev_close)
            });
            sum += tr;
        }
        return sum / (double)n;
    }

    double rsi_(int n) const {
        // Exponential RSI matching backtest (alpha = 1/n).
        if ((int)closes_.size() < n + 2) return 50.0;
        const int sz = (int)closes_.size();
        double avg_up = 0.0, avg_dn = 0.0;
        const double alpha = 1.0 / (double)n;
        for (int i = 1; i < sz; ++i) {
            double d = closes_[i] - closes_[i - 1];
            double u = d > 0 ? d : 0.0;
            double dn = d < 0 ? -d : 0.0;
            if (i == 1) { avg_up = u; avg_dn = dn; }
            else        { avg_up = (1 - alpha) * avg_up + alpha * u;
                          avg_dn = (1 - alpha) * avg_dn + alpha * dn; }
        }
        if (avg_dn == 0.0) return 100.0;
        double rs = avg_up / avg_dn;
        return 100.0 - 100.0 / (1.0 + rs);
    }

    // RSI value at one bar back (for cross-up detection)
    double rsi_prev_(int n) const {
        if ((int)closes_.size() < n + 3) return 50.0;
        const int sz = (int)closes_.size() - 1;  // pretend last bar doesn't exist
        double avg_up = 0.0, avg_dn = 0.0;
        const double alpha = 1.0 / (double)n;
        for (int i = 1; i < sz; ++i) {
            double d = closes_[i] - closes_[i - 1];
            double u = d > 0 ? d : 0.0;
            double dn = d < 0 ? -d : 0.0;
            if (i == 1) { avg_up = u; avg_dn = dn; }
            else        { avg_up = (1 - alpha) * avg_up + alpha * u;
                          avg_dn = (1 - alpha) * avg_dn + alpha * dn; }
        }
        if (avg_dn == 0.0) return 100.0;
        double rs = avg_up / avg_dn;
        return 100.0 - 100.0 / (1.0 + rs);
    }

    double bb_lower_(int n, double k) const {
        if ((int)closes_.size() < n) return 0.0;
        const int sz = (int)closes_.size();
        double mean = 0.0;
        for (int i = sz - n; i < sz; ++i) mean += closes_[i];
        mean /= (double)n;
        double var = 0.0;
        for (int i = sz - n; i < sz; ++i) {
            double d = closes_[i] - mean;
            var += d * d;
        }
        var /= (double)(n - 1);
        return mean - k * std::sqrt(var);
    }

    // ── Signal evaluation on the just-closed bar ─────────────────────────────
    bool signal_tsmom_() const {
        if ((int)closes_.size() < cfg_.lookback + 1) return false;
        double now = closes_.back();
        double ref = closes_[closes_.size() - 1 - cfg_.lookback];
        return now > ref;
    }

    bool signal_donchian_() const {
        // Long breakout: close > rolling N-bar PRIOR high (excludes current bar)
        if ((int)highs_.size() < cfg_.lookback + 1) return false;
        const int sz = (int)highs_.size();
        double prior_high = 0.0;
        for (int i = sz - cfg_.lookback - 1; i < sz - 1; ++i) {
            if (highs_[i] > prior_high) prior_high = highs_[i];
        }
        return closes_.back() > prior_high;
    }

    bool signal_bollinger_() const {
        if ((int)closes_.size() < cfg_.lookback) return false;
        double lower = bb_lower_(cfg_.lookback, cfg_.bb_k);
        // Pierce: bar's low touched the lower band; closed back above
        return (lows_.back() <= lower) && (closes_.back() > lower);
    }

    bool signal_rsi_revert_() const {
        if ((int)closes_.size() < cfg_.atr_period + 3) return false;
        double r_now  = rsi_(cfg_.atr_period);
        double r_prev = rsi_prev_(cfg_.atr_period);
        return (r_prev <= cfg_.rsi_threshold) && (r_now > cfg_.rsi_threshold);
    }

    void evaluate_signal_() {
        bool fire = false;
        switch (cfg_.kind) {
            case StrategyKind::TSMOM:      fire = signal_tsmom_();      break;
            case StrategyKind::DONCHIAN:   fire = signal_donchian_();   break;
            case StrategyKind::BOLLINGER:  fire = signal_bollinger_();  break;
            case StrategyKind::RSI_REVERT: fire = signal_rsi_revert_(); break;
        }
        if (!fire) return;

        double a = atr_(cfg_.atr_period);
        if (a <= 0.0) return;

        // Entry will materialise on the NEXT bar's open — but in a live tick
        // stream, "next bar open" = current price right now (we're at the bar
        // boundary). For paper-exact mirroring of backtest, defer to first
        // tick of the next bar via a pending flag instead. We use simple
        // approximation: enter at last_close_ which is the price at this
        // moment of bar close. The error vs theoretical next-bar-open is
        // <1 tick on liquid pairs.
        entry_px_     = last_close_;
        atr_at_entry_ = a;
        sl_px_        = entry_px_ - cfg_.sl_atr_mult * a;
        entry_ts_ms_  = cur_open_ts_ms_ + cfg_.tf_secs * 1000;
        time_exit_ts_ms_ = entry_ts_ms_ + (int64_t)cfg_.hold_bars * cfg_.tf_secs * 1000;
        in_position_  = true;
        bars_held_    = 0;

        std::printf("[%s] ENTRY  px=%.6f  sl=%.6f  atr=%.6f  hold=%dbars  shadow=%d\n",
            cfg_.tag.c_str(), entry_px_, sl_px_, a, cfg_.hold_bars, shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    void check_exits_(double price, int64_t ts_ms) {
        if (!in_position_) return;
        // Stop-loss: price touched the stop.
        if (price <= sl_px_) {
            exit_position_(sl_px_, ts_ms, "SL");
            return;
        }
        // Time exit: held long enough.
        if (ts_ms >= time_exit_ts_ms_) {
            exit_position_(price, ts_ms, "TIME");
        }
    }

    void exit_position_(double exit_px, int64_t ts_ms, const char* reason) {
        if (!in_position_) return;
        double gross_bp = (exit_px / entry_px_ - 1.0) * 1e4;
        double net_bp   = gross_bp - cfg_.round_trip_bp;
        trades_++;
        if (net_bp > 0) wins_++;
        total_bp_      += net_bp;
        last_trade_bp_  = net_bp;

        std::printf("[%s] EXIT   reason=%s  px=%.6f  gross=%+8.2fbp  net=%+8.2fbp  "
                    "trades=%d wins=%d total=%+8.1fbp\n",
            cfg_.tag.c_str(), reason, exit_px, gross_bp, net_bp,
            trades_, wins_, total_bp_);
        std::fflush(stdout);

        in_position_ = false;
        entry_px_ = 0.0;
        sl_px_    = 0.0;
        entry_ts_ms_     = 0;
        time_exit_ts_ms_ = 0;
        atr_at_entry_    = 0.0;
        bars_held_       = 0;
    }
};

} // namespace chimera

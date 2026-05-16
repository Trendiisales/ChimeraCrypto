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
//   - Trailing stop: arms at 1.0x ATR profit, trails at peak - 0.5x ATR
//     (ratchets up only, never down). Once armed, the effective stop is
//     max(hard_sl, trail_stop) — so the trail only helps, never hurts.
//   - Time exit at hold_bars after entry
//
// All instances are LONG-only spot (per ChimeraCrypto SPOT-ONLY guardrail).
// Shadow mode default = true; promote to live only after 4 weeks of paper
// trades match backtest expectations.
//
// Bar synthesis is internal: each engine accumulates ticks into its own
// timeframe bars (no shared bar bus required).
//
// Cold-start mitigation: seed_bars() pre-populates the closed-bar deques from
// historical OHLC pulled by main.cpp (BinanceREST::fetch_klines), so an engine
// can evaluate signals on bar 1 instead of waiting ~lookback bars for live
// ticks to build the history (which would take ~20 days for BTC-TSMOM-D1).
//
// Time-gated strategies (added 2026-05-16):
//   OVERNIGHT  — buy at 21:00 UTC bar close (H1) when trend is positive.
//                Captures the documented overnight premium (21-23 UTC window).
//   WEEKDAY    — buy on Monday D1 bar close when close > SMA(5).
//                Captures the Monday effect (+0.51%/day avg).
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
#include <ctime>
#include <functional>

namespace chimera {

enum class StrategyKind {
    TSMOM,       // 20-bar return > 0
    DONCHIAN,    // close > prior 20-bar high
    BOLLINGER,   // bar pierces lower BB(20,2) then closes back above
    RSI_REVERT,  // RSI(14) crosses up from <= 30
    OVERNIGHT,   // H1 bar at 21:00 UTC + uptrend filter
    WEEKDAY      // D1 bar on Monday + SMA(5) filter
};

inline const char* strategy_name(StrategyKind k) {
    switch (k) {
        case StrategyKind::TSMOM:      return "TSMOM";
        case StrategyKind::DONCHIAN:   return "DONCHIAN";
        case StrategyKind::BOLLINGER:  return "BOLLINGER";
        case StrategyKind::RSI_REVERT: return "RSI_REVERT";
        case StrategyKind::OVERNIGHT:  return "OVERNIGHT";
        case StrategyKind::WEEKDAY:    return "WEEKDAY";
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

        // ── Trailing stop parameters ──────────────────────────────────────
        // trail_arm_atr: profit (in ATR multiples) required before trail
        //   activates. E.g. 1.0 means price must reach entry + 1.0*ATR.
        // trail_dist_atr: once armed, trail sits this far below peak price
        //   (in ATR multiples). E.g. 0.5 means trail_stop = peak - 0.5*ATR.
        // Trail only ratchets UP. Effective stop = max(hard_sl, trail_stop).
        double       trail_arm_atr  = 1.0;
        double       trail_dist_atr = 0.5;

        // ── OVERNIGHT strategy parameters ────────────────────────────────
        // entry_hour_utc: the UTC hour at which the H1 bar must close for
        //   signal to fire. Default 21 = the 21:00-22:00 bar close.
        int          entry_hour_utc = 21;

        // ── WEEKDAY strategy parameters ──────────────────────────────────
        // entry_dow: day-of-week for entry (0=Sunday, 1=Monday, ..., 6=Saturday)
        int          entry_dow = 1;  // Monday
        // sma_len: SMA length for the momentum filter (close > SMA to enter)
        int          sma_len = 5;
    };

    // -----------------------------------------------------------------------
    // SeedBar — one historical OHLC bar supplied to seed_bars().
    // Decoupled from any specific REST client so EdgeEngine.hpp stays free
    // of curl/openssl includes. main.cpp converts BinanceREST::Kline to this.
    // -----------------------------------------------------------------------
    struct SeedBar {
        int64_t open_ts_ms = 0;
        double  o = 0.0;
        double  h = 0.0;
        double  l = 0.0;
        double  c = 0.0;
    };

    // -----------------------------------------------------------------------
    // TradeRecord — emitted via on_trade callback after every exit.
    // main.cpp persists these to disk for the dashboard trade history.
    // -----------------------------------------------------------------------
    struct TradeRecord {
        std::string tag;
        std::string symbol;
        std::string strategy;
        std::string reason;      // "SL", "TRAIL", "TIME", "KILL"
        int64_t     entry_ts_ms  = 0;
        int64_t     exit_ts_ms   = 0;
        double      entry_px     = 0.0;
        double      exit_px      = 0.0;
        double      sl_px        = 0.0;
        double      gross_bp     = 0.0;
        double      net_bp       = 0.0;
        double      mfe_bp       = 0.0;  // max favourable excursion
        int         trade_num    = 0;    // sequential trade number
        bool        shadow       = true;
    };

    using TradeCallback = std::function<void(const TradeRecord&)>;

    // -----------------------------------------------------------------------
    // BarRecord — emitted via on_bar callback after every completed bar.
    // main.cpp persists these to disk for warm-start and audit trail.
    // -----------------------------------------------------------------------
    struct BarRecord {
        std::string tag;
        int64_t     open_ts_ms  = 0;
        int64_t     tf_secs     = 0;
        double      o           = 0.0;
        double      h           = 0.0;
        double      l           = 0.0;
        double      c           = 0.0;
        double      atr         = 0.0;
        double      momentum_pct = 0.0;
        bool        signal_ready = false;
        bool        signal_fired = false;
        bool        in_position  = false;
        int         bars_in_buffer = 0;
    };

    using BarCallback = std::function<void(const BarRecord&)>;

    bool shadow_mode = true;  // public for main.cpp init parity with old engines

    // Set a callback to receive trade records on each exit.
    void set_on_trade(TradeCallback cb) { on_trade_ = std::move(cb); }

    // Set a callback to receive bar records on each bar close.
    void set_on_bar(BarCallback cb) { on_bar_ = std::move(cb); }

    explicit EdgeEngine(const Config& cfg) : cfg_(cfg) {
        if (cfg_.max_history < cfg_.lookback + 5)  cfg_.max_history = cfg_.lookback + 5;
        if (cfg_.max_history < cfg_.atr_period + 5) cfg_.max_history = cfg_.atr_period + 5;
        if (cfg_.max_history < cfg_.sma_len + 5)    cfg_.max_history = cfg_.sma_len + 5;
        std::printf("[%s] ARMED  symbol=%s strat=%s tf=%llds lookback=%d hold=%d sl=%.2f*atr trail_arm=%.1f*atr trail_dist=%.1f*atr  shadow=%d\n",
            cfg_.tag.c_str(), cfg_.symbol.c_str(),
            strategy_name(cfg_.kind),
            (long long)cfg_.tf_secs, cfg_.lookback, cfg_.hold_bars, cfg_.sl_atr_mult,
            cfg_.trail_arm_atr, cfg_.trail_dist_atr,
            shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    // -----------------------------------------------------------------------
    // seed_bars — pre-populate the closed-bar history from REST klines.
    //
    // Called once at startup before the live tick stream begins. Bars must
    // arrive OLDEST-FIRST (which is how Binance returns them). After seeding,
    // cur_bar_id_ is set to the last seeded bar's id so the next live tick
    // either extends the current (partial) bar or starts a fresh one cleanly.
    //
    // Returns the number of bars actually inserted (after max_history trim).
    // Safe to call with an empty vector (no-op).
    // -----------------------------------------------------------------------
    int seed_bars(const std::vector<SeedBar>& bars) {
        if (bars.empty()) return 0;

        for (const auto& b : bars) {
            if (b.o <= 0.0 || b.h <= 0.0 || b.l <= 0.0 || b.c <= 0.0) continue;
            opens_.push_back(b.o);
            highs_.push_back(b.h);
            lows_.push_back(b.l);
            closes_.push_back(b.c);
            bar_ts_ms_.push_back(b.open_ts_ms);
        }

        // Trim to max_history (drop oldest first).
        while ((int)closes_.size() > cfg_.max_history) {
            opens_.pop_front();
            highs_.pop_front();
            lows_.pop_front();
            closes_.pop_front();
            bar_ts_ms_.pop_front();
        }

        if (!closes_.empty()) {
            last_close_ = closes_.back();

            // Anchor cur_bar_id_ to the most recent SEEDED bar so the first
            // live tick after seeding doesn't fire close_bar_() with garbage.
            // open_ts_ms is the bar OPEN timestamp; the corresponding bar id
            // is (open_ts_ms / 1000) / tf_secs.
            int64_t last_open_ts_ms = bar_ts_ms_.back();
            cur_bar_id_     = last_open_ts_ms / 1000 / cfg_.tf_secs;
            cur_open_ts_ms_ = cur_bar_id_ * cfg_.tf_secs * 1000;

            // Initialise the in-flight bar at the last close so the first
            // live tick either updates the high/low/close of this same bar
            // (if still within its window) or rolls forward via the normal
            // gap-fill path in on_tick().
            cur_open_  = last_close_;
            cur_high_  = last_close_;
            cur_low_   = last_close_;
            cur_close_ = last_close_;
        }

        std::printf("[%s] SEED   bars_in=%d closes_kept=%d last_close=%.6f\n",
            cfg_.tag.c_str(),
            (int)bars.size(), (int)closes_.size(), last_close_);
        std::fflush(stdout);

        return (int)closes_.size();
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

    // JSON state line for /api/state (one object per engine; main.cpp wraps in array).
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
        js << "\"sl_px\":"     << (in_position_ ? effective_stop_() : 0.0) << ",";
        js << "\"last_close\":" << last_close_ << ",";
        js << "\"trades\":"    << trades_ << ",";
        js << "\"wins\":"      << wins_   << ",";
        js << std::setprecision(2);
        js << "\"total_bp\":"  << total_bp_      << ",";
        js << "\"last_bp\":"   << last_trade_bp_ << ",";
        js << "\"bars_in_buffer\":" << (int)closes_.size() << ",";
        // Trailing stop state for GUI
        js << "\"trail_armed\":" << (trail_armed_ ? "true" : "false") << ",";
        js << std::setprecision(6);
        js << "\"mfe_px\":" << (in_position_ ? mfe_px_ : 0.0) << ",";
        js << std::setprecision(2);
        js << "\"mfe_bp\":" << (in_position_ ? mfe_bp_ : 0.0) << ",";
        js << std::setprecision(6);
        js << "\"trail_stop_px\":" << (trail_armed_ ? trail_stop_px_ : 0.0) << ",";

        // ── Diagnostic fields (read-only, no effect on trading logic) ────
        js << "\"lookback\":" << cfg_.lookback << ",";
        js << "\"hold_bars_cfg\":" << cfg_.hold_bars << ",";
        js << "\"sl_atr_mult\":" << std::setprecision(1) << cfg_.sl_atr_mult << ",";
        js << "\"round_trip_bp\":" << std::setprecision(1) << cfg_.round_trip_bp << ",";
        js << "\"trail_arm_atr\":" << std::setprecision(1) << cfg_.trail_arm_atr << ",";
        js << "\"trail_dist_atr\":" << std::setprecision(1) << cfg_.trail_dist_atr << ",";

        // Momentum: close[now] vs close[now - lookback]
        bool signal_ready = ((int)closes_.size() >= cfg_.lookback + 1);
        double lb_close = 0.0;
        double momentum_pct = 0.0;
        if (signal_ready) {
            lb_close = closes_[closes_.size() - 1 - cfg_.lookback];
            if (lb_close > 0.0)
                momentum_pct = (closes_.back() / lb_close - 1.0) * 100.0;
        }
        js << "\"signal_ready\":" << (signal_ready ? "true" : "false") << ",";
        js << std::setprecision(4);
        js << "\"lookback_close\":" << lb_close << ",";
        js << "\"momentum_pct\":" << momentum_pct << ",";

        // Bar timing: when did the current bar open, when does it close?
        int64_t next_bar_close_ms = (cur_bar_id_ + 1) * cfg_.tf_secs * 1000;
        js << "\"cur_bar_open_ms\":" << cur_open_ts_ms_ << ",";
        js << "\"next_bar_close_ms\":" << next_bar_close_ms << ",";
        js << "\"bars_held\":" << bars_held_;

        js << "}";
        return js.str();
    }

    // Counters
    int trades() const { return trades_; }
    int wins() const { return wins_; }
    double total_bp() const { return total_bp_; }
    bool in_position() const { return in_position_; }
    int bars_in_buffer() const { return (int)closes_.size(); }

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
    double  sl_px_       = 0.0;     // hard stop-loss (never moves)
    int64_t entry_ts_ms_ = 0;
    int64_t time_exit_ts_ms_ = 0;
    double  atr_at_entry_ = 0.0;
    int     bars_held_ = 0;

    // Trailing stop state
    bool    trail_armed_    = false;
    double  trail_stop_px_  = 0.0;  // ratchets up, never down
    double  trail_arm_px_   = 0.0;  // price level that arms the trail
    double  mfe_px_         = 0.0;  // max favourable excursion (highest price seen)
    double  mfe_bp_         = 0.0;  // MFE in basis points from entry

    // Stats
    int    trades_ = 0;
    int    wins_   = 0;
    double total_bp_ = 0.0;
    double last_trade_bp_ = 0.0;
    bool   halted_ = false;

    // Trade callback (set by main.cpp for persistence)
    TradeCallback on_trade_;
    // Bar callback (set by main.cpp for bar persistence + warm-start)
    BarCallback on_bar_;

    // ── Effective stop: max(hard_sl, trail_stop) ─────────────────────────────
    double effective_stop_() const {
        if (trail_armed_ && trail_stop_px_ > sl_px_) return trail_stop_px_;
        return sl_px_;
    }

    // ── Helper: extract UTC hour from a millisecond epoch timestamp ──────────
    static int utc_hour_from_ms_(int64_t ts_ms) {
        time_t secs = static_cast<time_t>(ts_ms / 1000);
        struct tm utc;
        gmtime_r(&secs, &utc);
        return utc.tm_hour;
    }

    // ── Helper: extract day-of-week (0=Sun,1=Mon,...6=Sat) from ms epoch ────
    static int utc_dow_from_ms_(int64_t ts_ms) {
        time_t secs = static_cast<time_t>(ts_ms / 1000);
        struct tm utc;
        gmtime_r(&secs, &utc);
        return utc.tm_wday;
    }

    // ── Simple Moving Average of last n closes ──────────────────────────────
    double sma_(int n) const {
        if ((int)closes_.size() < n) return 0.0;
        double sum = 0.0;
        const int sz = (int)closes_.size();
        for (int i = sz - n; i < sz; ++i) sum += closes_[i];
        return sum / (double)n;
    }

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
        bool was_flat = !in_position_;
        if (!in_position_ && !halted_) {
            evaluate_signal_();
        }
        bool signal_fired = was_flat && in_position_;  // we just entered

        bars_held_ = in_position_ ? (bars_held_ + 1) : 0;

        // ── Fire bar callback for persistence + audit trail ──────────────
        if (on_bar_) {
            BarRecord br;
            br.tag            = cfg_.tag;
            br.open_ts_ms     = bar_ts_ms_.back();
            br.tf_secs        = cfg_.tf_secs;
            br.o              = opens_.back();
            br.h              = highs_.back();
            br.l              = lows_.back();
            br.c              = closes_.back();
            br.atr            = atr_(cfg_.atr_period);
            br.bars_in_buffer = (int)closes_.size();
            br.signal_ready   = ((int)closes_.size() >= cfg_.lookback + 1);
            br.signal_fired   = signal_fired;
            br.in_position    = in_position_;
            br.momentum_pct   = 0.0;
            if (br.signal_ready) {
                double lb_c = closes_[closes_.size() - 1 - cfg_.lookback];
                if (lb_c > 0.0) br.momentum_pct = (closes_.back() / lb_c - 1.0) * 100.0;
            }
            on_bar_(br);
        }
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

    // ── OVERNIGHT: buy at the entry_hour_utc H1 bar close when trend is up ──
    // Time gate: only fires when the just-closed bar's open hour matches
    //   cfg_.entry_hour_utc (default 21 = the 21:00-22:00 UTC bar).
    // Trend filter: TSMOM over lookback bars (close > close[lookback] ago).
    //   This prevents buying in downtrends where the overnight premium is
    //   consumed by the larger bearish drift.
    // Bullish bar filter: the bar itself must be green (close > open) to
    //   confirm buying pressure is present in the session window.
    bool signal_overnight_() const {
        if ((int)closes_.size() < cfg_.lookback + 1) return false;

        // Time gate: check UTC hour of the just-closed bar
        int64_t bar_open_ms = bar_ts_ms_.back();
        int bar_hour = utc_hour_from_ms_(bar_open_ms);
        if (bar_hour != cfg_.entry_hour_utc) return false;

        // Trend filter: 20-bar TSMOM must be positive
        double now = closes_.back();
        double ref = closes_[closes_.size() - 1 - cfg_.lookback];
        if (now <= ref) return false;

        // Bullish bar: this bar's close > open
        if (closes_.back() <= opens_.back()) return false;

        std::printf("[%s] OVERNIGHT signal | bar_hour=%d(UTC) | trend_ret=+%.1fbp | bar_ret=+%.1fbp\n",
            cfg_.tag.c_str(), bar_hour,
            (now / ref - 1.0) * 1e4,
            (closes_.back() / opens_.back() - 1.0) * 1e4);
        std::fflush(stdout);

        return true;
    }

    // ── WEEKDAY: buy on Monday D1 bar close when close > SMA(sma_len) ───────
    // Time gate: only fires when the just-closed D1 bar falls on entry_dow
    //   (default 1 = Monday).
    // Momentum filter: close must be above SMA(sma_len) to confirm the
    //   underlying trend supports the Monday premium.
    bool signal_weekday_() const {
        if ((int)closes_.size() < cfg_.sma_len) return false;

        // Time gate: check day-of-week of the just-closed bar
        int64_t bar_open_ms = bar_ts_ms_.back();
        int bar_dow = utc_dow_from_ms_(bar_open_ms);
        if (bar_dow != cfg_.entry_dow) return false;

        // Momentum filter: close > SMA
        double sma = sma_(cfg_.sma_len);
        if (sma <= 0.0) return false;
        if (closes_.back() <= sma) return false;

        std::printf("[%s] WEEKDAY signal | dow=%d | close=%.2f > sma(%d)=%.2f\n",
            cfg_.tag.c_str(), bar_dow, closes_.back(), cfg_.sma_len, sma);
        std::fflush(stdout);

        return true;
    }

    void evaluate_signal_() {
        bool fire = false;
        switch (cfg_.kind) {
            case StrategyKind::TSMOM:      fire = signal_tsmom_();      break;
            case StrategyKind::DONCHIAN:   fire = signal_donchian_();   break;
            case StrategyKind::BOLLINGER:  fire = signal_bollinger_();  break;
            case StrategyKind::RSI_REVERT: fire = signal_rsi_revert_(); break;
            case StrategyKind::OVERNIGHT:  fire = signal_overnight_();  break;
            case StrategyKind::WEEKDAY:    fire = signal_weekday_();    break;
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

        // Initialise trailing stop state
        trail_armed_   = false;
        trail_stop_px_ = 0.0;
        trail_arm_px_  = entry_px_ + cfg_.trail_arm_atr * a;
        mfe_px_        = entry_px_;
        mfe_bp_        = 0.0;

        double arm_bp = (trail_arm_px_ / entry_px_ - 1.0) * 1e4;
        std::printf("[%s] ENTRY  px=%.6f  sl=%.6f  atr=%.6f  hold=%dbars  trail_arm=%.6f(+%.0fbp)  shadow=%d\n",
            cfg_.tag.c_str(), entry_px_, sl_px_, a, cfg_.hold_bars,
            trail_arm_px_, arm_bp,
            shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    void check_exits_(double price, int64_t ts_ms) {
        if (!in_position_) return;

        // Update MFE tracking
        if (price > mfe_px_) {
            mfe_px_ = price;
            mfe_bp_ = (price / entry_px_ - 1.0) * 1e4;
        }

        // Trailing stop logic: arm when price reaches the arm level,
        // then ratchet the trail stop up as price makes new highs.
        if (!trail_armed_) {
            if (price >= trail_arm_px_) {
                trail_armed_ = true;
                trail_stop_px_ = mfe_px_ - cfg_.trail_dist_atr * atr_at_entry_;
                double trail_bp = (trail_stop_px_ / entry_px_ - 1.0) * 1e4;
                std::printf("[%s] TRAIL_ARM  px=%.6f  mfe=%.6f(+%.1fbp)  trail_stop=%.6f(+%.1fbp)\n",
                    cfg_.tag.c_str(), price, mfe_px_, mfe_bp_,
                    trail_stop_px_, trail_bp);
                std::fflush(stdout);
            }
        } else {
            // Ratchet: if price made a new high, update the trail stop
            double new_trail = mfe_px_ - cfg_.trail_dist_atr * atr_at_entry_;
            if (new_trail > trail_stop_px_) {
                trail_stop_px_ = new_trail;
            }
        }

        // Exit check: use effective stop (max of hard SL and trail stop)
        double eff_stop = effective_stop_();
        if (price <= eff_stop) {
            const char* reason = (trail_armed_ && trail_stop_px_ >= sl_px_) ? "TRAIL" : "SL";
            exit_position_(eff_stop, ts_ms, reason);
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
                    "mfe=%+.1fbp  trades=%d wins=%d total=%+8.1fbp\n",
            cfg_.tag.c_str(), reason, exit_px, gross_bp, net_bp,
            mfe_bp_, trades_, wins_, total_bp_);
        std::fflush(stdout);

        // Fire trade callback for persistence
        if (on_trade_) {
            TradeRecord rec;
            rec.tag         = cfg_.tag;
            rec.symbol      = cfg_.symbol;
            rec.strategy    = strategy_name(cfg_.kind);
            rec.reason      = reason;
            rec.entry_ts_ms = entry_ts_ms_;
            rec.exit_ts_ms  = ts_ms;
            rec.entry_px    = entry_px_;
            rec.exit_px     = exit_px;
            rec.sl_px       = sl_px_;
            rec.gross_bp    = gross_bp;
            rec.net_bp      = net_bp;
            rec.mfe_bp      = mfe_bp_;
            rec.trade_num   = trades_;
            rec.shadow      = shadow_mode;
            on_trade_(rec);
        }

        in_position_ = false;
        entry_px_ = 0.0;
        sl_px_    = 0.0;
        entry_ts_ms_     = 0;
        time_exit_ts_ms_ = 0;
        atr_at_entry_    = 0.0;
        bars_held_       = 0;

        // Reset trailing stop state
        trail_armed_    = false;
        trail_stop_px_  = 0.0;
        trail_arm_px_   = 0.0;
        mfe_px_         = 0.0;
        mfe_bp_         = 0.0;
    }
};

} // namespace chimera

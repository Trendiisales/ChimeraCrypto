#pragma once
#include "live/BinanceWSFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "config/TradingConfig.hpp"
#include "LatencyGovernor.hpp"
#include "RegimeTypes.hpp"
#include "LeadLagEngine.hpp"
#include "core/VolumeShockEngine.hpp"
#include "LiquidationEngine.hpp"
#include "VolatilityScoring.hpp"
#include "StatefulGovernor.hpp"
#include "MultiSymbolAllocator.hpp"
#include "RejectionTelemetryAsync.hpp"
#include "execution/PnLByLatencyBand.hpp"
#include "execution/EngineStallDetector.hpp"
#include "GuiMessageBuilder.hpp"

// PHASE 2: Institutional microstructure and capital allocation
#include "market_data/BookState.hpp"
#include "market_data/MarketEnv.hpp"
#include "microstructure/ToxicFlowDetector.hpp"
#include "microstructure/MicroEdgeEngine.hpp"
#include "microstructure/HybridRegimeClassifier.hpp"
#include "allocation/AdaptiveAllocator.hpp"
#include "allocation/CapitalControlLayer.hpp"
#include "execution/ExecutionOptimizer.hpp"
#include "reinforcement/AdaptiveReinforcementLayer.hpp"
#include "market_data/FundingRateFetcher.hpp"
#include "core/NGASLeadLagEngine.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <string>
#include <functional>
#include <deque>
#include <sstream>
#include <iomanip>
#include "logging/ShadowLogger.hpp"
#include "core/LimitOrderManager.hpp"
#include "core/PnLGovernor.hpp"

namespace chimera {

enum LayerMode {
    LAYER_NONE,
    LAYER_MICRO,
    LAYER_IMPULSE,
    LAYER_EXPANSION,
    LAYER_LEADLAG,
    LAYER_LEADLAG_ETH_SOL,  // ETH leads SOL  secondary correlation
    LAYER_VACUUM,        // Liquidity Vacuum  ask-side drain breakout
    LAYER_VWAP,          // VWAP Reversion  buy dip back to session VWAP
    LAYER_LIQUIDATION,   // Liquidation Cascade  short liq on perp  spot long
    LAYER_FUNDING,       // Funding Rate Signal  deeply negative funding  sustained long
    LAYER_NGAS,          // NGAS Lead-Lag  Natural Gas drop  risk-on crypto LONG
    LAYER_ETH_LEAD,      // ETH leads SOL/BNB/AVAX/LINK/POL (Tier 2 lead-lag)
    LAYER_SOL_LEAD,      // SOL leads AVAX/POL (Tier 3 lead-lag)
    LAYER_VOLSHOCK,         // Volume Shock Continuation -- spike + displacement
    LAYER_OFI,              // Order Flow Imbalance -- persistent buy aggression
    LAYER_SWEEP,            // Liquidity Sweep -- aggressive spike + depth collapse
    LAYER_MM_PRESSURE       // Market Maker Inventory Pressure -- slow drift + absorption
};

enum PosState {
    POS_FLAT,
    POS_PENDING,  // Limit order posted, waiting for fill
    POS_OPEN
};

enum SystemState {
    SYS_IDLE,
    SYS_IMPULSE,
    SYS_EXPANSION
};

struct Position {
    PosState state;
    double entry_price;
    int64_t entry_ts;
    LayerMode layer;
    bool is_long = true;     // Trade direction: true=LONG, false=SHORT
    int open_ticks;      // Ticks since position opened (for minimum hold time)
    double peak_price;   // Highest favorable price since entry (for trailing)
    
    // Phase 2: MFE/MAE tracking
    double mfe;
    double mae;

    // Maker limit order fields
    LayerMode pending_layer = LAYER_NONE;  // layer that triggered the limit

    void reset() {
        state = POS_FLAT;
        entry_price = 0.0;
        entry_ts = 0;
        layer = LAYER_NONE;
        open_ticks = 0;
        peak_price = 0.0;
        mfe = 0.0;
        mae = 0.0;
        pending_layer = LAYER_NONE;
    }
};

struct SymbolState {
    double last_price;
    std::deque<double> short_returns;  // Rolling log returns for short window
    std::deque<double> long_returns;   // Rolling log returns for long window (deprecated - keeping for backwards compat)
    
    // EMA-based long volatility tracking (adaptive baseline)
    double long_vol_ema;   // Exponential moving average of volatility
    bool ema_initialized;  // Flag to track if EMA has been seeded
    
    // EMA-based vol_ratio smoothing (reduces tick-to-tick noise in regime classification)
    double vol_ratio_ema;      // Smoothed volatility ratio
    bool ratio_ema_initialized;  // Flag to track if ratio EMA has been seeded

    Position pos;
    int64_t cooldown_until;
    Regime regime;
    int regime_ticks;            // Ticks since last regime change (for hysteresis)
    double regime_anchor_price;  // Price when regime last changed (for displacement confirmation)

    MarketTick last_tick;   // Latest tick with real book data

    // Entry context for shadow logger  captured at entry, written at exit
    double entry_imbalance  = 0.0;
    double entry_flow_ratio = 0.0;
    double entry_spread_bps = 0.0;
    double entry_btc_move   = 0.0;
    double entry_latency_ms = 0.0;

    // Session VWAP tracking (reset each day / on first tick)
    double vwap_cum_pv   = 0.0;   // cumulative price * volume
    double vwap_cum_vol  = 0.0;   // cumulative volume
    double session_vwap  = 0.0;   // current VWAP value
    int64_t vwap_session_start = 0; // timestamp of session start

    // Liquidity Vacuum: rolling ask depth baseline
    double ask_depth_ema = 0.0;   // EMA of ask_size (baseline)
    bool ask_depth_init  = false;

    // Funding signal: track last entry time per symbol
    int64_t last_funding_entry_ts = 0;

    // NGAS lead-lag: track last entry time per symbol
    int64_t last_ngas_entry_ts = 0;

    // Aggressive flow EMAs — used for LEADLAG confirmation gate
    // Alpha=0.05 → ~20-tick window (~200-500ms at typical tick rate)
    double buy_vol_ema  = 0.0;   // EMA of agg_buy_volume per tick
    double sell_vol_ema = 0.0;   // EMA of agg_sell_volume per tick
    bool   flow_ema_init = false;

    // ── OFI Pressure: longer-window buy/sell volume accumulator ────────────
    // Alpha=0.03 → ~33-tick window.  Separate from the fast 0.05 LEADLAG gate.
    double ofi_buy_ema   = 0.0;
    double ofi_sell_ema  = 0.0;
    bool   ofi_ema_init  = false;

    // ── Sweep detection: trade-size EMA + prior-tick depth snapshot ────────
    double trade_size_ema   = 0.0;   // EMA(alpha=0.05) of per-tick trade_qty
    bool   trade_size_init  = false;
    double prev_ask_depth   = 0.0;   // depth snapshot from previous tick
    double prev_bid_depth   = 0.0;

    // ── MM Inventory Pressure: slow book-imbalance + price-drift EMAs ──────
    double mm_imbal_ema     = 0.0;   // EMA(alpha=0.02) of book_imbalance
    bool   mm_imbal_init    = false;
    double mm_drift_sum     = 0.0;   // cumulative mid-price change over window
    double mm_prev_mid      = 0.0;   // previous mid for drift calculation
    int    mm_drift_ticks   = 0;     // ticks in current drift window

    void reset() {
        last_price = 0;
        short_returns.clear();
        long_returns.clear();
        long_vol_ema = 0.0;
        ema_initialized = false;
        vol_ratio_ema = 0.0;
        ratio_ema_initialized = false;
        cooldown_until = 0;
        regime = REGIME_DEAD;
        regime_ticks = 0;
        regime_anchor_price = 0;
        vwap_cum_pv = 0.0;
        vwap_cum_vol = 0.0;
        session_vwap = 0.0;
        vwap_session_start = 0;
        ask_depth_ema = 0.0;
        ask_depth_init = false;
        buy_vol_ema   = 0.0;
        sell_vol_ema  = 0.0;
        flow_ema_init = false;
        ofi_buy_ema   = 0.0;
        ofi_sell_ema  = 0.0;
        ofi_ema_init  = false;
        trade_size_ema  = 0.0;
        trade_size_init = false;
        prev_ask_depth  = 0.0;
        prev_bid_depth  = 0.0;
        mm_imbal_ema    = 0.0;
        mm_imbal_init   = false;
        mm_drift_sum    = 0.0;
        mm_prev_mid     = 0.0;
        mm_drift_ticks  = 0;
        pos.reset();
    }
};

struct RejectionThrottle {
    std::unordered_map<std::string, int> rejection_counts;
    std::unordered_map<std::string, std::string> last_rejection_reason;
    int64_t last_summary_ts = 0;
    
    void record(const std::string& key, const std::string& reason) {
        rejection_counts[key]++;
        
        if (last_rejection_reason[key] != reason) {
            last_rejection_reason[key] = reason;
            std::printf("[SIGNAL-DIAG] %s | reason=%s (will suppress repeats)\n", key.c_str(), reason.c_str());
            std::fflush(stdout);
        }
    }
    
    void print_summary(int64_t ts) {
        if (ts - last_summary_ts < 60000) return;
        
        if (!rejection_counts.empty()) {
            std::printf("\n[REJECTION-SUMMARY] Last 60s:\n");
            for (const auto& kv : rejection_counts) {
                std::printf("  %s: %d rejections (%s)\n", 
                           kv.first.c_str(), kv.second, last_rejection_reason[kv.first].c_str());
            }
            std::printf("\n");
            std::fflush(stdout);
        }
        
        rejection_counts.clear();
        last_summary_ts = ts;
    }
};

// =============================================================================
// LayerPerformanceTracker
// Per-layer exponential moving average of PnL (bps).
// Adapted from the EdgeReinforcement pattern in the reference engine designs.
// alpha=0.05 → ~20-trade window. Multiplier clamped [0.4, 1.5].
// OFI/SWEEP/MM_PRESSURE start at 0.5x per config; once they accumulate
// 10+ trades the tracker overrides with a data-driven multiplier.
// =============================================================================
class LayerPerformanceTracker {
public:
    static constexpr int NUM_LAYERS = 17;
    static constexpr double ALPHA   = 0.05;   // ~20-trade EMA window
    static constexpr double MULT_LO = 0.40;   // floor: never below 40% base size
    static constexpr double MULT_HI = 1.50;   // ceiling: conservative until proven
    static constexpr int    MIN_TRADES = 10;  // trades required before override kicks in

    void record(LayerMode layer, double pnl_bps) {
        int idx = (int)layer;
        if (idx <= 0 || idx >= NUM_LAYERS) return;
        pnl_ema_[idx] = pnl_ema_[idx] * (1.0 - ALPHA) + pnl_bps * ALPHA;
        trade_count_[idx]++;
    }

    // Returns a sizing multiplier in [MULT_LO, MULT_HI].
    // If fewer than MIN_TRADES have been seen for this layer, returns 1.0
    // (neutral — let the hard-coded eng_mult in enter() control sizing).
    double multiplier(LayerMode layer) const {
        int idx = (int)layer;
        if (idx <= 0 || idx >= NUM_LAYERS) return 1.0;
        if (trade_count_[idx] < MIN_TRADES) return 1.0;
        // Linearly map EMA pnl in [-20, +20] bps to [MULT_LO, MULT_HI]
        double norm = pnl_ema_[idx] / 20.0;  // normalise: +20bp → +1.0
        double mult = 1.0 + norm * 0.5;      // ±50% swing around 1.0
        return std::max(MULT_LO, std::min(MULT_HI, mult));
    }

    double pnl_ema(LayerMode layer) const {
        int idx = (int)layer;
        return (idx > 0 && idx < NUM_LAYERS) ? pnl_ema_[idx] : 0.0;
    }

    int trade_count(LayerMode layer) const {
        int idx = (int)layer;
        return (idx > 0 && idx < NUM_LAYERS) ? trade_count_[idx] : 0;
    }

private:
    double pnl_ema_[NUM_LAYERS]   = {};
    int    trade_count_[NUM_LAYERS] = {};
};

class BalancedEngine {
public:
    using GuiBroadcastCallback = std::function<void(const std::string&)>;

    // Called after every completed trade  for QuadEngine to log full trade data
    struct TradeExitData {
        std::string symbol;   // "BTC" / "ETH" / "SOL"
        std::string engine;   // "EXPAND" / "LEADLAG" etc.
        double pnl_bp;
        double entry_price;
        double exit_price;
        double mfe_bp;
        double mae_bp;
        int64_t hold_ms;
        std::string reason;   // "TP" / "SL" / "TRAIL" / "TIMEOUT"
    };
    using TradeExitCallback = std::function<void(const TradeExitData&)>;

    void set_trade_exit_callback(TradeExitCallback cb) { trade_exit_cb_ = std::move(cb); }
    
    BalancedEngine() : governor_(GovernorConfig()), allocator_(AllocatorConfig()), gui_broadcast_(nullptr) {
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            symbols_[i].reset();
        open_positions_ = 0;
        loss_streak_ = 0;
        kill_until_ = 0;
        system_state_ = SYS_IDLE;
        layer_lock_until_ = 0;
        total_pnl_ = 0.0;
        realized_pnl_ = 0.0;
        total_trades_ = 0;
        consecutive_losses_ = 0;
        last_loss_ts_ = 0;
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            last_snapshot_update_[i] = 0;
            tick_count_[i] = 0;
            last_tick_count_reset_[i] = 0;
            snapshots_[i].symbol = sym_full(i);
            snapshots_[i].last_disable_time = std::chrono::steady_clock::now();
        }
        
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            expand_state_[i] = 0;
            expand_entry_price_[i] = 0.0;
            expand_peak_price_[i] = 0.0;
            expand_confirm_ticks_[i] = 0;
            expand_post_compress_ticks_[i] = 999; // start above lockout  no initial block
            sym_consecutive_sl_[i] = 0;
            sym_sl_cooldown_[i]    = 0;
            depth_baseline_[i]     = 0.0;
        }
        
        // Phase 2: Initialize capital control
        capital_control_.set_base_capital(10000.0);
        
        std::printf("\n");
        std::printf("      BALANCED ENGINE PHASE 2 - INSTITUTIONAL REBUILD          \n");
        std::printf("\n");
        std::printf(" Microstructure Analysis: ENABLED                              \n");
        std::printf(" Toxic Flow Detection: ENABLED                                 \n");
        std::printf(" Hybrid Regime Classifier: ENABLED                             \n");
        std::printf(" Adaptive Allocator: ENABLED (5ms loop)                        \n");
        std::printf(" Capital Control Layer: ENABLED                                \n");
        std::printf(" Execution Optimizer: ENABLED                                  \n");
        std::printf(" Reinforcement Layer: ENABLED                                  \n");
        std::printf(" Engines: LEADLAG BTCETH/SOL | ETHSOL | IMPULSE | EXPAND    \n");
        std::printf("          VACUUM | VWAP-REV | IMBAL (6 engines, long-only)     \n");
        std::printf(" Multi-position: UP TO 3 (1 per symbol)                        \n");
        std::printf(" Dead zone (20-23 UTC): max 1 pos, raised thresholds           \n");
        std::printf("\n");
        std::fflush(stdout);
    }

    inline void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        stall_detector_.on_ws_receive();
        stall_detector_.on_eval_start();
        
        std::string my_symbol = sym_full(id);
        rejection_telemetry_.recordEvaluation(my_symbol);
        
        // Update depth baseline for real queue_density in cap_env
        double depth = tick.bid_size + tick.ask_size;
        if (depth_baseline_[id] < 1e-9) depth_baseline_[id] = depth;
        else depth_baseline_[id] = depth_baseline_[id] * 0.995 + depth * 0.005;
        
        // PHASE 2: Update microstructure engines
        update_market_data(id, tick, ts, latency_ms);
        // Only update last_tick when bookTicker data is present (bid > 0)
        // aggTrade fires with bid=0 and would corrupt the stored book state
        if (tick.bid > 0.0 && tick.ask > 0.0) {
            symbols_[id].last_tick = tick;
        } else if (symbols_[id].last_tick.bid > 0.0) {
            // Merge: keep book state, update only trade flow fields
            symbols_[id].last_tick.last_price     = tick.last_price;
            symbols_[id].last_tick.trade_qty      = tick.trade_qty;
            symbols_[id].last_tick.agg_buy_volume = tick.agg_buy_volume;
            symbols_[id].last_tick.agg_sell_volume= tick.agg_sell_volume;
            symbols_[id].last_tick.is_buyer_maker = tick.is_buyer_maker;
            symbols_[id].last_tick.trade_time     = tick.trade_time;
            symbols_[id].last_tick.timestamp      = tick.timestamp;
            symbols_[id].last_tick.rtt_ms         = tick.rtt_ms;
        } else {
            symbols_[id].last_tick = tick; // no book yet, take whatever we have
        }
        
        // Periodic reporting
        static int64_t last_report_ts = 0;
        if (ts - last_report_ts > 60000) {
            std::printf("%s", pnl_by_band_.generate_report().c_str());
            std::printf("%s", stall_detector_.generate_report().c_str());
            
            // Phase 2: Report microstructure state
            report_phase2_metrics();
            
            for (const auto& kv : rejection_throttle_.rejection_counts) {
                broadcast_to_gui(GuiMessageBuilder::rejection_summary(
                    kv.first, kv.second, rejection_throttle_.last_rejection_reason[kv.first]
                ));
            }
            
            std::printf("[REJECTION-SUMMARY] Last 60s:\n");
            rejection_throttle_.print_summary(ts);
            
            std::string pnl_json = pnl_by_band_.build_json();
            broadcast_to_gui("{\"type\":\"latency_band_pnl\",\"data\":" + pnl_json + "}");
            
            broadcast_to_gui(GuiMessageBuilder::engine_health(
                stall_detector_.max_event_loop_delay_us.load(std::memory_order_relaxed) / 1000.0,
                stall_detector_.max_eval_duration_us.load(std::memory_order_relaxed) / 1000.0,
                static_cast<int>(stall_detector_.stall_events.load(std::memory_order_relaxed)),
                static_cast<int>(stall_detector_.samples.load(std::memory_order_relaxed))
            ));
            
            // Broadcast performance summary
            broadcast_to_gui(GuiMessageBuilder::performance_summary(
                10000.0 + total_pnl_,  // Base equity + realized PnL
                total_pnl_,
                total_trades_,
                open_positions_,
                loss_streak_
            ));
            
            // Broadcast comprehensive telemetry for GUI dashboard
            std::ostringstream telem;
            telem << std::fixed << std::setprecision(2);
            telem << "{\"type\":\"telemetry\","
                  << "\"equity\":" << (10000.0 + total_pnl_) << ","
                  << "\"day_pnl\":" << total_pnl_ << ","
                  << "\"pnl\":" << total_pnl_ << ","
                  << "\"unrealized_pnl\":0,"
                  << "\"trades_today\":" << total_trades_ << ","
                  << "\"positions\":" << open_positions_ << ","
                  << "\"exposure_usd\":0,"
                  << "\"win_rate\":" << (total_trades_ > 0 ? 1.0 - (double)consecutive_losses_ / total_trades_ : 0.0) << ","
                  << "\"sharpe_ratio\":0.0,"
                  << "\"btc_position\":0,"
                  << "\"eth_position\":0,"
                  << "\"sol_position\":0,"
                  << "\"governor\":\"ACTIVE\"," 
                  << "\"boost_leadlag\":" << capital_control_.win_boost_for("LEADLAG") << "," 
                  << "\"boost_ll_eth_sol\":" << capital_control_.win_boost_for("LL-ETH-SOL") << "," 
                  << "\"boost_impulse\":" << capital_control_.win_boost_for("IMPULSE") << "," 
                  << "\"boost_expand\":" << capital_control_.win_boost_for("EXPAND") << "," 
                  << "\"boost_liq\":" << capital_control_.win_boost_for("LIQ") << "," 
                  << "\"boost_fund\":" << capital_control_.win_boost_for("FUND") << "," 
                  << "\"boost_ngas\":" << capital_control_.win_boost_for("NGAS") 
                  << "}";
            broadcast_to_gui(telem.str());
            
            std::fflush(stdout);
            last_report_ts = ts;
        }
        
        // Position state diagnostic
        static int64_t last_pos_diag = 0;
        if (ts - last_pos_diag > 10000) {
            std::printf("[POSITION-STATE] open=%d | sys_state=%d | loss_streak=%d | kill_until=%ld\n",
                       open_positions_, system_state_, loss_streak_, 
                       (kill_until_ > ts ? (kill_until_ - ts) : 0));
            
            for (int i = 0; i < MAX_SYMBOLS; ++i) {
                const char* sym = sym_short(i);
                const char* regime_name_str = regime_classifiers_[i].regime_name();
                std::printf("[SYMBOL-STATE] %s | regime=%s (score=%.2f) | pos=%s | toxicity=%.2f | short_n=%zu | long_n=%zu\n",
                           sym, regime_name_str, regime_classifiers_[i].regime_score(),
                           (symbols_[i].pos.state == POS_OPEN ? "OPEN" : "FLAT"),
                           toxic_flow_[i].toxicity(),
                           symbols_[i].short_returns.size(), symbols_[i].long_returns.size());
                
                // Compute current volatilities for GUI broadcast
                double curr_short_vol = 0.0;
                double curr_long_vol = 0.0;
                if (symbols_[i].short_returns.size() >= TradingConfig::SHORT_VOL_WINDOW) {
                    double mean = 0.0;
                    for (double v : symbols_[i].short_returns) mean += v;
                    mean /= symbols_[i].short_returns.size();
                    double var = 0.0;
                    for (double v : symbols_[i].short_returns) var += (v - mean) * (v - mean);
                    curr_short_vol = std::sqrt(var / symbols_[i].short_returns.size());
                }
                if (symbols_[i].long_returns.size() >= TradingConfig::LONG_VOL_WINDOW) {
                    double mean = 0.0;
                    for (double v : symbols_[i].long_returns) mean += v;
                    mean /= symbols_[i].long_returns.size();
                    double var = 0.0;
                    for (double v : symbols_[i].long_returns) var += (v - mean) * (v - mean);
                    curr_long_vol = std::sqrt(var / symbols_[i].long_returns.size());
                }
                
                std::string symbol_full_str = chimera::sym_full(i);
                broadcast_to_gui(GuiMessageBuilder::regime_update(
                    symbol_full_str, symbols_[i].regime, symbols_[i].last_price,
                    (int)symbols_[i].short_returns.size(), curr_short_vol, curr_long_vol
                ));
            }
            
            broadcast_to_gui(GuiMessageBuilder::performance_summary(
                10000.0 + total_pnl_, total_pnl_, total_trades_, 
                open_positions_, loss_streak_
            ));
            
            std::fflush(stdout);
            last_pos_diag = ts;
        }
        
        latency_gov_.update(latency_ms, ts);
        leadlag_.update_price(id, price, ts);
        vol_scoring_[id].update(price, ts);
        
        tick_count_[id]++;
        if (ts - last_tick_count_reset_[id] >= 1000) {
            snapshots_[id].ticks_per_sec = tick_count_[id];
            tick_count_[id] = 0;
            last_tick_count_reset_[id] = ts;
        }
        
        snapshots_[id].last_price = price;
        if (snapshots_[id].ref_price == 0.0) {
            snapshots_[id].ref_price = price;
        }
        
        // ---- INSTITUTIONAL VOLATILITY MODEL (LOG RETURN BASED) ----
        auto& s = symbols_[id];
        
        // Compute log return
        double r = 0.0;
        if (s.last_price > 0.0) {
            r = std::log(price / s.last_price);
        }
        s.last_price = price;

        // ---- SESSION VWAP UPDATE ----
        // Reset VWAP at UTC midnight (session boundary)
        int64_t day_ms = ts % 86400000LL;
        if (s.vwap_session_start == 0 || day_ms < 1000) {
            s.vwap_cum_pv  = 0.0;
            s.vwap_cum_vol = 0.0;
            s.vwap_session_start = ts;
        }
        double vol_tick = (tick.trade_qty > 0.0) ? tick.trade_qty : 1.0;
        s.vwap_cum_pv  += price * vol_tick;
        s.vwap_cum_vol += vol_tick;
        if (s.vwap_cum_vol > 0.0)
            s.session_vwap = s.vwap_cum_pv / s.vwap_cum_vol;

        // ---- ASK DEPTH EMA (for Liquidity Vacuum baseline) ----
        if (tick.ask_size > 0.0) {
            if (!s.ask_depth_init) {
                s.ask_depth_ema = tick.ask_size;
                s.ask_depth_init = true;
            } else {
                s.ask_depth_ema = 0.02 * tick.ask_size + 0.98 * s.ask_depth_ema;
            }
        }
        
        // Push into short window (rolling window for short-term volatility)
        s.short_returns.push_back(r);
        if (s.short_returns.size() > TradingConfig::SHORT_VOL_WINDOW)
            s.short_returns.pop_front();
        
        // Update long_vol using EMA instead of rolling window
        // This makes long_vol adaptive instead of inert
        if (s.short_returns.size() >= TradingConfig::SHORT_VOL_WINDOW) {
            double current_short_vol = compute_volatility(s.short_returns);
            
            if (!s.ema_initialized) {
                // Seed EMA with first valid short_vol measurement
                s.long_vol_ema = current_short_vol;
                s.ema_initialized = true;
            } else {
                // Update EMA: long_vol = alpha * current + (1-alpha) * previous
                s.long_vol_ema = TradingConfig::LONG_VOL_EMA_ALPHA * current_short_vol + 
                                (1.0 - TradingConfig::LONG_VOL_EMA_ALPHA) * s.long_vol_ema;
            }
        }
        
        // Keep old long_returns for backward compatibility (but not used anymore)
        s.long_returns.push_back(r);
        if (s.long_returns.size() > TradingConfig::LONG_VOL_WINDOW)
            s.long_returns.pop_front();
        
        // Update regime based on volatility ratio
        (void)(s.regime);  // suppress unused-variable for old_regime
        s.regime = classify_regime(id);
        // Note: classify_regime() already logs the change with full detail
        
        // Position management
        // PENDING: limit order posted, check for fill or cancellation
        if (s.pos.state == POS_PENDING) {
            manage_pending(id, price, ts, s);
            return;
        }

        if (s.pos.state == POS_OPEN) {
            // Track MFE/MAE
            double move = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
            s.pos.mfe = std::max(s.pos.mfe, move);
            s.pos.mae = std::min(s.pos.mae, move);
            
            manage_position(id, price, ts, s);
            return;
        }
        
        // Signal evaluation
        pnl_governor_.reset_if_new_day();  // BUG2 FIX: daily PnL reset check
        if (ts < kill_until_) return;
        if (ts < s.cooldown_until) return;
        // Per-symbol guard: don't enter if this symbol already has a position
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return;

        // Time-of-day session gating
        int utc_hour = (int)((ts / 3600000LL) % 24);
        bool dead_zone = (utc_hour >= TradingConfig::SESSION_DEAD_START_UTC &&
                          utc_hour <  TradingConfig::SESSION_DEAD_END_UTC);
        const bool shadow_mode = (executor_ && executor_->is_shadow());
        int max_pos = dead_zone ? TradingConfig::DEAD_ZONE_MAX_POS : TradingConfig::MAX_CONCURRENT_POSITIONS;
        if (shadow_mode) {
            // Shadow research mode: do not throttle opportunity discovery too hard.
            max_pos = std::max(max_pos, 4);
        }
        if (open_positions_ >= max_pos) return;
        
        // PER-SYMBOL CIRCUIT BREAKER  block entry if symbol is in SL cooldown
        // Fires after SYM_SL_STREAK_LIMIT (2) consecutive SL losses on this symbol
        // Prevents entering a trending-against-us move (e.g. ETH crash 02:46-02:51)
        if (ts < sym_sl_cooldown_[id]) {
            // Only log once per 30 seconds to avoid spam
            static int64_t last_cb_log_[MAX_SYMBOLS] = {};
            if (ts - last_cb_log_[id] > 30000) {
                std::printf("[CIRCUIT-BREAK] %s | paused %.0fs remaining (SL streak=%d)\n",
                    sym_short(id),
                    (sym_sl_cooldown_[id] - ts) / 1000.0, sym_consecutive_sl_[id]);
                std::fflush(stdout);
                last_cb_log_[id] = ts;
            }
            return;
        }

        // Try signals in priority order
        // Priority: liquidation first (strongest signal), then lead-lag, then breakout, then microstructure
        if (try_liquidation_entry(id, price, ts, s, latency_ms)) return;
        if (try_funding_entry(id, price, ts, s, latency_ms)) return;
        if (try_ngas_entry(id, price, ts, s, latency_ms)) return;
        bool ll_prime = (utc_hour >= TradingConfig::LEADLAG_PRIME_START_UTC &&
                         utc_hour <  TradingConfig::LEADLAG_PRIME_END_UTC);
        ll_offpeak_size_mult_ = ll_prime ? 1.0 : TradingConfig::LEADLAG_OFFPEAK_SIZE_MULT;
        if (check_leadlag(id, price, ts, s, latency_ms)) return;
        if (check_leadlag_eth_sol(id, price, ts, s, latency_ms)) return;
        // DISABLED: ETH-LEAD 17% WR, net -121bp across 6 trades
        // if (check_eth_lead(id, price, ts, s, latency_ms)) return;
        // DISABLED: SOL-LEAD 0% WR, insufficient data, net -17bp
        // if (check_sol_lead(id, price, ts, s, latency_ms)) return;
        // Keep LIVE profile conservative, but allow these paths in SHADOW research mode.
        if (shadow_mode && check_impulse(id, price, ts, s, latency_ms)) return;
        if (check_vol_shock(id, price, ts, s, latency_ms)) return;
        if (shadow_mode && check_expansion(id, price, ts, s, latency_ms)) return;
        // SHADOW fallback: controlled micro-probe in flat/compression tape.
        // Purpose: keep sample throughput non-zero when high-conviction layers are quiet.
        // LIVE mode never uses this path.
        if (shadow_mode) {
            const MarketTick& t = s.last_tick;
            const bool cooldown_ok = (ts - shadow_probe_last_ms_[id]) >= 20000;
            const bool vol_ok = (s.vol_ratio_ema >= 0.75 && s.vol_ratio_ema <= 2.20);
            if (cooldown_ok && vol_ok) {
                bool spread_ok = true;
                bool pressure_ok = true;
                if (t.bid > 0.0 && t.ask > 0.0) {
                    const double flow = compute_flow_ratio(id);
                    spread_ok = (t.spread_bps > 0.0 && t.spread_bps <= 6.0);
                    pressure_ok = (t.book_imbalance >= -0.05 && flow >= 0.48);
                }
                if (spread_ok && pressure_ok) {
                    shadow_probe_last_ms_[id] = ts;
                    rejection_throttle_.record(std::string(sym_short(id)) + " SHADOW-PROBE", "fired");
                    enter(id, price, ts, s, LAYER_MICRO, true);
                    return;
                }
            }
        }

        // Book-dependent engines require a valid top-of-book snapshot.
        // During warm-up, aggTrade ticks can arrive before first bookTicker.
        const bool has_book = (s.last_tick.bid > 0.0 && s.last_tick.ask > 0.0);
        if (has_book) {
            if (check_vacuum(id, price, ts, s, latency_ms)) return;
            if (check_imbalance(id, price, ts, s, latency_ms)) return;
            if (check_vwap_reversion(id, price, ts, s, latency_ms)) return;
            if (check_ofi_pressure(id, price, ts, s, latency_ms)) return;
            if (check_sweep(id, price, ts, s, latency_ms)) return;
            if (check_mm_pressure(id, price, ts, s, latency_ms)) return;
        }
    }
    
    std::string get_rejection_stats() const { return rejection_telemetry_.build_json_snapshot(); }
    // Boost multiplier values exposed for /api/state polling
    std::string get_boost_json() const {
        std::ostringstream j;
        j << std::fixed << std::setprecision(4);
        j << "\"boost_leadlag\":"    << capital_control_.win_boost_for("LEADLAG")    << ",";
        j << "\"boost_ll_eth_sol\":" << capital_control_.win_boost_for("LL-ETH-SOL") << ",";
        j << "\"boost_impulse\":"    << capital_control_.win_boost_for("IMPULSE")    << ",";
        j << "\"boost_expand\":"     << capital_control_.win_boost_for("EXPAND")     << ",";
        j << "\"boost_liq\":"        << capital_control_.win_boost_for("LIQ")        << ",";
        j << "\"boost_fund\":"       << capital_control_.win_boost_for("FUND")       << ",";
        j << "\"boost_ngas\":"       << capital_control_.win_boost_for("NGAS")       << ",";
        j << "\"boost_eth_lead\":"   << capital_control_.win_boost_for("ETH-LEAD")   << ",";
        j << "\"boost_sol_lead\":"   << capital_control_.win_boost_for("SOL-LEAD")   << ",";
        j << "\"boost_volshock\":"   << capital_control_.win_boost_for("VOLSHOCK") << ",";
        j << "\"boost_ofi\":"         << capital_control_.win_boost_for("OFI")        << ",";
        j << "\"boost_sweep\":"       << capital_control_.win_boost_for("SWEEP")      << ",";
        j << "\"boost_mm\":"          << capital_control_.win_boost_for("MM-PRESSURE");
        return j.str();
    }
    void set_funding_fetcher(FundingRateFetcher* f) { funding_ = f; }
    void set_ngas_engine(NGASLeadLagEngine* n)     { ngas_ = n; }
    void set_executor(SpotExecutor* e)              { executor_ = e; }
    LiquidationEngine& liq_engine()                 { return liq_engine_; }
    double get_total_pnl()      const { return total_pnl_; }
    double get_realized_pnl()   const { return realized_pnl_; }
    double get_win_boost_for(const std::string& engine) const {
        return capital_control_.win_boost_for(engine);
    }
    int    get_total_trades()   const { return total_trades_; }
    int    get_open_positions() const { return open_positions_; }

    // Per-layer adaptive sizing state — for GUI layer-adapt panel
    // Returns JSON fragment (no outer braces) ready to embed in telemetry payload
    std::string get_layer_adapt_json() const {
        std::ostringstream j;
        j << std::fixed << std::setprecision(3);
        // Only the three new layers need monitoring — established layers are not tracker-driven
        static const struct { LayerMode mode; const char* name; } TRACKED[] = {
            { LAYER_OFI,         "ofi"  },
            { LAYER_SWEEP,       "sweep"},
            { LAYER_MM_PRESSURE, "mm"   },
        };
        j << "\"layer_adapt\":{";
        bool first = true;
        for (const auto& t : TRACKED) {
            if (!first) j << ",";
            first = false;
            j << "\"" << t.name << "\":{"
              << "\"trades\":"  << layer_tracker_.trade_count(t.mode) << ","
              << "\"pnl_ema\":" << layer_tracker_.pnl_ema(t.mode) << ","
              << "\"mult\":"    << layer_tracker_.multiplier(t.mode)
              << "}";
        }
        j << "}";
        return j.str();
    }

    // Full session breakdown  auto-maintained on every exit, no grep needed
    std::string get_session_stats_json() const {
        static const char* LAYER_NAMES[] = {
            "NONE","MICRO","IMPULSE","EXPAND","LEADLAG","LL-ETH-SOL","VACUUM","VWAP","LIQ","FUND","NGAS","ETH-LEAD","SOL-LEAD","VOLSHOCK","OFI","SWEEP","MM-PRESSURE"
        };
        std::ostringstream j;
        j << std::fixed << std::setprecision(2);
        int total_wins=0,total_losses=0,total_tp=0,total_sl=0,total_trail=0,total_timeout=0;
        double total_pnl=0.0;
        for (int i=1;i<17;i++){  // 1=MICRO .. 16=MM_PRESSURE — covers all active layers
            total_wins    += layer_stats_[i].wins;
            total_losses  += layer_stats_[i].losses;
            total_tp      += layer_stats_[i].tp_exits;
            total_sl      += layer_stats_[i].sl_exits;
            total_trail   += layer_stats_[i].trail_exits;
            total_timeout += layer_stats_[i].timeout_exits;
            total_pnl     += layer_stats_[i].total_pnl_bp;
        }
        int total = total_wins + total_losses;
        j << "\"session\":{"
          << "\"total_trades\":"  << total << ","
          << "\"wins\":"          << total_wins << ","
          << "\"losses\":"        << total_losses << ","
          << "\"win_rate\":"      << (total>0?(double)total_wins/total*100.0:0.0) << ","
          << "\"total_pnl_bp\":"  << total_pnl << ","
          << "\"tp_exits\":"      << total_tp << ","
          << "\"sl_exits\":"      << total_sl << ","
          << "\"trail_exits\":"   << total_trail << ","
          << "\"timeout_exits\":" << total_timeout << ","
          << "\"by_layer\":[";
        bool first=true;
        for (int i=1;i<17;i++){  // 1=MICRO .. 16=MM_PRESSURE -- covers all active layers
            const auto& ls=layer_stats_[i];
            if (ls.total()==0) continue;
            if (!first) j << ",";
            first=false;
            j << "{\"name\":\"" << LAYER_NAMES[i] << "\","
              << "\"trades\":"  << ls.total() << ","
              << "\"wins\":"    << ls.wins << ","
              << "\"losses\":"  << ls.losses << ","
              << "\"wr\":"      << ls.win_rate() << ","
              << "\"pnl\":"     << ls.total_pnl_bp << ","
              << "\"avg_pnl\":" << ls.avg_pnl() << ","
              << "\"best\":"    << ls.best_trade << ","
              << "\"worst\":"   << ls.worst_trade << ","
              << "\"avg_mfe\":" << ls.avg_mfe() << ","
              << "\"avg_mae\":" << ls.avg_mae() << ","
              << "\"tp\":"      << ls.tp_exits << ","
              << "\"sl\":"      << ls.sl_exits << ","
              << "\"trail\":"   << ls.trail_exits << ","
              << "\"timeout\":" << ls.timeout_exits << "}";
        }
        j << "]}";
        return j.str();
    }
    
    void set_gui_broadcast(GuiBroadcastCallback callback) {
        // DISABLED - GUI decoupled, logs only
        // gui_broadcast_ = callback;
    }
    
private:
    // PHASE 2: Market data update
    // tick carries REAL bid/ask/depth/trade data from the live feed.
    // No fake constants. If a field is 0.0, the feed hasn't sent it yet 
    // we use it as-is; the EMA-based engines self-initialise gracefully.
    void update_market_data(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        // Keep snapshot latency current so exit() can log it correctly
        snapshots_[id].lat_p95_ms = latency_ms;

        double short_vol = compute_volatility(symbols_[id].short_returns);
        double long_vol  = compute_volatility(symbols_[id].long_returns);

        // ---- ToxicFlowDetector ----
        // Needs: trade direction volumes, book depth, spread, vol ranges
        ToxicFlowDetector::TickInput toxic_input;
        toxic_input.trade_volume           = tick.trade_qty;
        toxic_input.aggressive_buy_volume  = tick.agg_buy_volume;
        toxic_input.aggressive_sell_volume = tick.agg_sell_volume;
        toxic_input.bid_depth              = tick.bid_size;
        toxic_input.ask_depth              = tick.ask_size;
        toxic_input.spread_bps             = tick.spread_bps;
        toxic_input.short_range            = short_vol;
        toxic_input.long_range             = long_vol;
        // Only update when we have real trade data (agg volumes non-zero)
        if (tick.trade_qty > 0.0) {
            toxic_flow_[id].update(toxic_input);
        }

        // ---- MicroEdgeEngine ----
        // Needs: bid/ask depth, spread, mid, vol ranges, funding (spot = 0)
        MicroEdgeEngine::BookState micro_state;
        micro_state.bid_depth    = tick.bid_size;
        micro_state.ask_depth    = tick.ask_size;
        micro_state.spread_bps   = tick.spread_bps;
        micro_state.mid_price    = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        micro_state.short_range  = short_vol;
        micro_state.long_range   = long_vol;
        micro_state.funding_rate = 0.0;   // Spot market - no funding rate
        micro_state.latency_ms   = latency_ms;
        // Only update when we have real book data
        if (tick.bid > 0.0 && tick.ask > 0.0) {
            micro_edge_[id].update(micro_state);
        }

        // ---- HybridRegimeClassifier ----
        // Needs: all of the above combined
        HybridRegimeClassifier::Input regime_input;
        regime_input.short_range             = short_vol;
        regime_input.long_range              = long_vol;
        regime_input.trade_volume            = tick.trade_qty;
        regime_input.aggressive_buy_volume   = tick.agg_buy_volume;
        regime_input.aggressive_sell_volume  = tick.agg_sell_volume;
        regime_input.bid_depth               = tick.bid_size;
        regime_input.ask_depth               = tick.ask_size;
        regime_input.spread_bps              = tick.spread_bps;
        regime_input.latency_ms              = latency_ms;
        // Update on every tick (vol ranges always valid after warm-up)
        regime_classifiers_[id].update(regime_input);

        // ---- MarketEnv (cross-symbol aggregate) ----
        double vol0 = compute_volatility(symbols_[0].short_returns);
        double vol1 = compute_volatility(symbols_[1].short_returns);
        double vol2 = compute_volatility(symbols_[2].short_returns);
        market_env_.short_range = (vol0 + vol1 + vol2) / 3.0;

        double lvol0 = compute_volatility(symbols_[0].long_returns);
        double lvol1 = compute_volatility(symbols_[1].long_returns);
        double lvol2 = compute_volatility(symbols_[2].long_returns);
        market_env_.long_range  = (lvol0 + lvol1 + lvol2) / 3.0;
        market_env_.vol_ratio   = market_env_.short_range / std::max(market_env_.long_range, 1e-6);
        market_env_.spread_bps  = tick.spread_bps;
        market_env_.book_imbalance = tick.book_imbalance;
        market_env_.latency_ms  = latency_ms;
        market_env_.net_clean   = (latency_ms < TradingConfig::LATENCY_NET_CLEAN_MS);

        // ---- AdaptiveAllocator ----
        AdaptiveAllocator::Environment alloc_env;
        alloc_env.short_range = market_env_.short_range;
        alloc_env.long_range  = market_env_.long_range;
        alloc_env.spread_bps  = tick.spread_bps;
        alloc_env.latency_ms  = latency_ms;
        alloc_env.net_clean   = market_env_.net_clean;
        adaptive_allocator_.tick(alloc_env);

        // Update aggressive flow EMAs for LEADLAG confirmation gate
        // Alpha=0.05 → ~20-tick smoothing window
        constexpr double FLOW_EMA_ALPHA = 0.05;
        if (!symbols_[id].flow_ema_init) {
            symbols_[id].buy_vol_ema  = tick.agg_buy_volume;
            symbols_[id].sell_vol_ema = tick.agg_sell_volume;
            symbols_[id].flow_ema_init = true;
        } else {
            symbols_[id].buy_vol_ema  = FLOW_EMA_ALPHA * tick.agg_buy_volume
                                       + (1.0 - FLOW_EMA_ALPHA) * symbols_[id].buy_vol_ema;
            symbols_[id].sell_vol_ema = FLOW_EMA_ALPHA * tick.agg_sell_volume
                                       + (1.0 - FLOW_EMA_ALPHA) * symbols_[id].sell_vol_ema;
        }

        // ── OFI slow EMA (alpha=0.03, ~33-tick window) ────────────────────────
        if (tick.agg_buy_volume > 0.0 || tick.agg_sell_volume > 0.0) {
            constexpr double OFI_ALPHA = 0.03;
            auto& ss = symbols_[id];
            if (!ss.ofi_ema_init) {
                ss.ofi_buy_ema  = tick.agg_buy_volume;
                ss.ofi_sell_ema = tick.agg_sell_volume;
                ss.ofi_ema_init = true;
            } else {
                ss.ofi_buy_ema  = OFI_ALPHA * tick.agg_buy_volume
                                  + (1.0 - OFI_ALPHA) * ss.ofi_buy_ema;
                ss.ofi_sell_ema = OFI_ALPHA * tick.agg_sell_volume
                                  + (1.0 - OFI_ALPHA) * ss.ofi_sell_ema;
            }
        }

        // ── Sweep: trade-size EMA (alpha=0.05) ────────────────────────────────
        if (tick.trade_qty > 0.0) {
            constexpr double SZ_ALPHA = 0.05;
            auto& ss = symbols_[id];
            if (!ss.trade_size_init) {
                ss.trade_size_ema  = tick.trade_qty;
                ss.trade_size_init = true;
            } else {
                ss.trade_size_ema  = SZ_ALPHA * tick.trade_qty
                                     + (1.0 - SZ_ALPHA) * ss.trade_size_ema;
            }
        }

        // ── MM Pressure: slow book-imbalance EMA + mid-price drift ────────────
        if (tick.bid > 0.0 && tick.ask > 0.0) {
            constexpr double MM_ALPHA = 0.02;
            auto& ss = symbols_[id];
            double mid = (tick.bid + tick.ask) * 0.5;
            if (!ss.mm_imbal_init) {
                ss.mm_imbal_ema  = tick.book_imbalance;
                ss.mm_prev_mid   = mid;
                ss.mm_imbal_init = true;
            } else {
                ss.mm_imbal_ema  = MM_ALPHA * tick.book_imbalance
                                   + (1.0 - MM_ALPHA) * ss.mm_imbal_ema;
                // Accumulate drift: signed mid-price change
                ss.mm_drift_sum   += (mid - ss.mm_prev_mid);
                ss.mm_prev_mid    = mid;
                ss.mm_drift_ticks++;
                // Reset drift window every 100 ticks
                if (ss.mm_drift_ticks >= 100) {
                    ss.mm_drift_sum   = 0.0;
                    ss.mm_drift_ticks = 0;
                }
            }
        }

        // ── Sweep: snapshot current depth for next-tick collapse detection ────
        if (tick.bid > 0.0 && tick.ask > 0.0) {
            symbols_[id].prev_ask_depth = tick.ask_size;
            symbols_[id].prev_bid_depth = tick.bid_size;
        }
    }
    
    void report_phase2_metrics() {
        std::printf("\n[PHASE2-METRICS] Allocation & Microstructure:\n");
        std::printf("  Allocator: Impulse=%.3f | Maker=%.3f\n",
                   adaptive_allocator_.impulse_weight(),
                   adaptive_allocator_.maker_weight());
        
        for (int i = 0; i < 3; ++i) {
            const char* sym = sym_short(i);
            std::printf("  %s: Regime=%s (%.2f) | Tox=%.2f | Imp=%.2f | Mkr=%.2f\n",
                       sym,
                       regime_classifiers_[i].regime_name(),
                       regime_classifiers_[i].regime_score(),
                       toxic_flow_[i].toxicity(),
                       micro_edge_[i].impulse_bias(),
                       micro_edge_[i].maker_bias());
        }
        std::printf("\n");
    }
    
    void broadcast_to_gui(const std::string& message) {
        // DISABLED - GUI decoupled, using logs only
        // No WebSocket, no broadcast, just pure trading
        return;
    }
    
    const char* regime_name(Regime r) const {
        switch(r) {
            case REGIME_DEAD: return "DEAD";
            case REGIME_GRIND: return "GRIND";
            case REGIME_BUILDUP: return "BUILDUP";
            case REGIME_BREAKOUT: return "BREAKOUT";
            default: return "UNKNOWN";
        }
    }
    
    // Helper: Compute standard deviation from log returns
    double compute_volatility(const std::deque<double>& returns) const {
        if (returns.empty()) return 0.0;
        double mean = 0.0;
        for (double v : returns) mean += v;
        mean /= returns.size();
        double var = 0.0;
        for (double v : returns) var += (v - mean) * (v - mean);
        return std::sqrt(var / returns.size());
    }
    
    Regime classify_regime(int id) {
        auto& s = symbols_[id];
        
        // Increment regime stability counter
        s.regime_ticks++;
        
        // Need minimum data - wait for EMA to initialize
        if (!s.ema_initialized || s.short_returns.size() < TradingConfig::SHORT_VOL_WINDOW) 
            return REGIME_DEAD;
        
        // Compute short volatility (standard deviation of log returns from rolling window)
        double short_vol = compute_volatility(s.short_returns);
        
        // Use EMA-based long_vol (adaptive baseline)
        double long_vol = s.long_vol_ema;
        
        // Compute raw volatility ratio
        double vol_ratio_raw = 0.0;
        if (long_vol > TradingConfig::VOL_MIN_LONG)
            vol_ratio_raw = short_vol / long_vol;
        
        // Apply EMA smoothing to vol_ratio to reduce tick-to-tick noise
        if (!s.ratio_ema_initialized) {
            s.vol_ratio_ema = vol_ratio_raw;
            s.ratio_ema_initialized = true;
        } else {
            s.vol_ratio_ema = TradingConfig::VOL_RATIO_EMA_ALPHA * vol_ratio_raw + 
                             (1.0 - TradingConfig::VOL_RATIO_EMA_ALPHA) * s.vol_ratio_ema;
        }
        
        // Use smoothed vol_ratio for regime classification
        double vol_ratio = s.vol_ratio_ema;
        
        // INSTRUMENTATION - Print every 500 classifications
        static int diag_counter = 0;
        if (++diag_counter % TradingConfig::REGIME_DIAG_INTERVAL == 0) {
            const char* sym = sym_short(id);
            std::printf("[REGIME-RAW] %s | short_vol=%.6f | long_vol_ema=%.6f | vol_ratio_smooth=%.3f | regime=%s | ticks=%d\n",
                       sym, short_vol, long_vol, vol_ratio, regime_name(s.regime), s.regime_ticks);
            std::fflush(stdout);
        }
        
        // ENFORCE MINIMUM PERSISTENCE - Prevent thrashing
        if (s.regime_ticks < TradingConfig::MIN_REGIME_TICKS) {
            return s.regime;  // Hold current regime
        }
        
        // HYSTERESIS STATE MACHINE - Different thresholds for entering vs exiting regimes
        Regime new_regime = s.regime;
        
        switch (s.regime) {
            case REGIME_DEAD:
                // Exit DEAD only if ratio rises above 0.90
                if (vol_ratio > TradingConfig::REGIME_DEAD_EXIT)
                    new_regime = REGIME_GRIND;
                break;
                
            case REGIME_GRIND:
                // Exit to BUILDUP if ratio > 1.45
                if (vol_ratio > TradingConfig::REGIME_GRIND_EXIT_TO_BUILDUP)
                    new_regime = REGIME_BUILDUP;
                // Exit to DEAD only if ratio < 0.75
                else if (vol_ratio < TradingConfig::REGIME_GRIND_EXIT_TO_DEAD)
                    new_regime = REGIME_DEAD;
                break;
                
            case REGIME_BUILDUP:
                // Exit to BREAKOUT if ratio > 1.95
                if (vol_ratio > TradingConfig::REGIME_BUILDUP_TO_BREAKOUT)
                    new_regime = REGIME_BREAKOUT;
                // Exit to GRIND only if ratio < 1.10
                else if (vol_ratio < TradingConfig::REGIME_BUILDUP_EXIT)
                    new_regime = REGIME_GRIND;
                break;
                
            case REGIME_BREAKOUT:
                // Exit to BUILDUP only if ratio < 1.55
                if (vol_ratio < TradingConfig::REGIME_BREAKOUT_EXIT)
                    new_regime = REGIME_BUILDUP;
                break;
        }
        
        // If regime changed, reset tick counter and log
        if (new_regime != s.regime) {
            const char* sym = sym_short(id);
            // Suppress noisy DEADGRIND oscillation during startup calibration
            // Only log after the symbol has been running for at least 200 ticks
            bool suppress = (s.regime_ticks < 200 && 
                            (s.regime == REGIME_DEAD || new_regime == REGIME_DEAD));
            if (!suppress) {
                std::printf("[REGIME-CHANGE] %s: %s  %s (vol_ratio=%.3f after %d ticks)\n",
                           sym, regime_name(s.regime), regime_name(new_regime), vol_ratio, s.regime_ticks);
                std::fflush(stdout);
            }
            s.regime_ticks = 0;

            // Track COMPRESSIONBREAKOUT transition for EXPAND post-compress lockout
            // Prevents EXPAND from firing on the first ticks of compression exit
            // (regime lag: compression just ended but market hasn't confirmed direction)
            if (new_regime == REGIME_BREAKOUT) {
                expand_post_compress_ticks_[id] = 0;  // reset  start counting
            }

            // Set anchor price when entering BUILDUP or BREAKOUT for displacement confirmation
            if (new_regime == REGIME_BUILDUP || new_regime == REGIME_BREAKOUT) {
                s.regime_anchor_price = s.last_price;
            }
        }

        // Increment post-compress counter every tick while in BREAKOUT
        if (new_regime == REGIME_BREAKOUT && expand_post_compress_ticks_[id] < 9999) {
            expand_post_compress_ticks_[id]++;
        }
        
        return new_regime;
    }
    
    // ======================================================================
    // flow_ratio  directional order flow confirmation
    // ======================================================================
    // Uses real agg_buy_volume / agg_sell_volume from @aggTrade stream.
    // Returns fraction of recent volume that was aggressive buy: 0.5 = neutral
    // > 0.6 = buy pressure confirming a long signal
    // < 0.4 = sell pressure  do not enter long
    //
    // This is FREE information already in our feed that most systems ignore.
    // A breakout with 70% buy flow is far more reliable than one with 40%.
    // ======================================================================
    double compute_flow_ratio(int id) const {
        // Use the most recent tick's agg volumes
        const MarketTick& t = symbols_[id].last_tick;
        double buy  = t.agg_buy_volume;
        double sell = t.agg_sell_volume;
        double total = buy + sell;
        if (total < 1e-9) return 0.5; // no data  neutral
        return buy / total;
    }

    // ======================================================================
    // IMPULSE  fires in BREAKOUT regime
    // Latency requirement: < 50ms (hard limit)
    // Edge: genuine vol expansion, price displaced from regime anchor
    // TP/SL from TradingConfig: 20bp TP, 8bp SL
    // ======================================================================
    bool check_impulse(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = sym_short(id);
        std::string key = std::string(sym) + " IMPULSE";

        // SYMBOL FILTER for IMPULSE:
        // BTC(0): 56.7% WR +0.53bp/trade -- deep books eat the edge, 20% MFE capture, -2.72bp avg MAE. Not worth it.
        // ETH(1): 54% WR -10bp -- net loser
        // BNB(3): 28% WR -17bp -- active money destruction
        // WINNERS: SOL(2), AVAX(4), LINK(5), POL(6) -- thin books, explosive moves, TP in <1s
        if (id == 0 || id == 1 || id == 3) {
            rejection_throttle_.record(key, "symbol_filtered");
            return false;
        }

        // Per-symbol guard: don't enter if this symbol already has a position
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        if (latency_ms > TradingConfig::LATENCY_HARD_LIMIT_MS) {
            rejection_throttle_.record(key, "high_latency");
            return false;
        }
        if (s.regime != REGIME_BREAKOUT) {
            rejection_throttle_.record(key, "no_breakout");
            return false;
        }
        if ((int)s.short_returns.size() < TradingConfig::IMPULSE_MIN_SHORT_TICKS) {
            rejection_throttle_.record(key, "insufficient_ticks");
            return false;
        }

        double long_vol = s.long_vol_ema;
        if (long_vol < TradingConfig::MIN_LONG_VOL_FOR_TRADING) {
            rejection_throttle_.record(key, "low_vol");
            return false;
        }

        double displacement = std::abs(price - s.regime_anchor_price);
        if (displacement < TradingConfig::MIN_DISPLACEMENT_LONG_MULT * long_vol) {
            rejection_throttle_.record(key, "insufficient_displacement");
            return false;
        }

        // ORDER FLOW CONFIRMATION
        // Require majority of recent volume to be in the signal direction.
        // Breakout with opposing flow = likely false breakout / stop hunt.
        double flow = compute_flow_ratio(id);
        if (flow < TradingConfig::FLOW_CONFIRM_THRESHOLD) {
            rejection_throttle_.record(key, "weak_flow");
            return false;
        }

        enter(id, price, ts, s, LAYER_IMPULSE, true);
        return true;
    }

    // ======================================================================
    // EXPANSION  fires in BUILDUP or BREAKOUT regime
    // Latency requirement: < 50ms (hard limit)
    // Edge: vol_ratio expanding, price confirming direction
    // ======================================================================
    bool check_expansion(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = sym_short(id);
        std::string key = std::string(sym) + " EXPAND";

        // EXPAND symbol filter:
        //   BTC(0):  60% WR +13.65bp -- deep books, confirmed edge
        //   SOL(2):  blocked -- 60% WR but inverted R:R (-8.75bp losers vs +4.27bp winners)
        //   AVAX(4), LINK(5), POL(6): thin books, explosive moves, 3 live trades +22/+11/+22bp
        //     EXPAND requires REGIME_BREAKOUT (vol_ratio > 1.75x) -- very high bar on thin books
        //     When LINK/AVAX/POL reach BREAKOUT regime they run hard. EXPANSION_ALT_TP_BP=25bp.
        //   ETH(1), BNB(3): remain blocked -- net negative EV
        if (id == 1 || id == 2 || id == 3) {
            rejection_throttle_.record(key, "symbol_filtered");
            return false;
        }

        // Per-symbol guard
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        if (expand_state_[id] == 1) {
            rejection_throttle_.record(key, "already_in_expand");
            return false;
        }
        if (s.regime != REGIME_BREAKOUT) {
            // Was: BUILDUP || BREAKOUT  BUILDUP entries were failing too often.
            // BREAKOUT-only: vol_ratio already > 1.65, genuine expansion confirmed.
            rejection_throttle_.record(key, "weak_regime");
            return false;
        }

        double short_vol = compute_volatility(s.short_returns);
        double long_vol  = s.long_vol_ema;

        if (long_vol < TradingConfig::MIN_LONG_VOL_FOR_TRADING) {
            rejection_throttle_.record(key, "low_vol");
            return false;
        }

        double vol_ratio = (long_vol > TradingConfig::VOL_MIN_LONG) ? (short_vol / long_vol) : 0.0;
        if (vol_ratio <= TradingConfig::EXPANSION_VOL_RATIO) {
            expand_confirm_ticks_[id] = 0;  // reset consecutive counter on weak tick
            rejection_throttle_.record(key, "weak_volatility");
            return false;
        }

        // CONSECUTIVE TICK CONFIRMATION  require N ticks above threshold before entry
        // One tick above vol_ratio is noise. N consecutive ticks = genuine expansion.
        expand_confirm_ticks_[id]++;
        if (expand_confirm_ticks_[id] < TradingConfig::EXPANSION_CONFIRM_TICKS) {
            rejection_throttle_.record(key, "confirm_ticks_pending");
            return false;
        }

        // POST-COMPRESS LOCKOUT  block EXPAND for N ticks after COMPRESSIONBREAKOUT
        // Prevents firing on the regime lag period where compression just ended
        // but price hasn't yet picked a direction
        if (expand_post_compress_ticks_[id] < TradingConfig::EXPAND_POST_COMPRESS_LOCKOUT) {
            rejection_throttle_.record(key, "post_compress_lockout");
            return false;
        }

        if ((int)s.short_returns.size() < TradingConfig::EXPANSION_MIN_SHORT_TICKS) {
            rejection_throttle_.record(key, "insufficient_ticks");
            return false;
        }

        double displacement = std::abs(price - s.regime_anchor_price);
        if (displacement < TradingConfig::MIN_DISPLACEMENT_LONG_MULT * long_vol) {
            rejection_throttle_.record(key, "insufficient_displacement");
            return false;
        }

        // LATENCY GUARD  expansion edge decays fast, stale data = bad entry
        if (latency_ms > TradingConfig::LATENCY_NET_CLEAN_MS) {
            rejection_throttle_.record(key, "latency_too_high");
            return false;
        }

        // ORDER FLOW CONFIRMATION
        double flow = compute_flow_ratio(id);
        if (flow < TradingConfig::FLOW_CONFIRM_THRESHOLD) {
            rejection_throttle_.record(key, "weak_flow");
            return false;
        }

        expand_confirm_ticks_[id] = 0;  // reset after entry  fresh confirmation needed next time
        enter(id, price, ts, s, LAYER_EXPANSION, true);
        return true;
    }

    // ======================================================================
    // IMBALANCE  fires in GRIND regime on strong book pressure
    // Latency requirement: < 25ms (tighter than hard limit  needs fresh data)
    // Edge: bid/ask imbalance predicts 1-3 tick direction
    //
    // EV analysis at our latency:
    //   TP = 12bp gross (+2bp net after 10bp costs)
    //   SL = 3bp
    //   Needs ~80% win rate to be positive EV
    //   Threshold 0.45 filters to highest-quality setups
    // ======================================================================
    bool check_imbalance(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = sym_short(id);
        std::string key = std::string(sym) + " IMBAL";

        // Per-symbol guard
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        // TIGHTER latency gate than hard limit  imbalance edge decays fast
        if (latency_ms > TradingConfig::LATENCY_IMBALANCE_MAX_MS) {
            rejection_throttle_.record(key, "latency_too_high");
            return false;
        }
        if (s.regime != REGIME_GRIND) {
            rejection_throttle_.record(key, "not_grind");
            return false;
        }

        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0) {
            rejection_throttle_.record(key, "no_book_data");
            return false;
        }
        // Spread gate: tight spread = good fills. Wide = skip.
        if (t.spread_bps > TradingConfig::IMBALANCE_MAX_SPREAD_BPS) {
            rejection_throttle_.record(key, "spread_wide");
            return false;
        }

        double imbalance = t.book_imbalance;
        // Dead zone (20-23 UTC): raise threshold to filter noise in thin markets
        int utc_hour_imbal = (int)((ts / 3600000LL) % 24);
        bool dead_imbal = (utc_hour_imbal >= TradingConfig::SESSION_DEAD_START_UTC &&
                           utc_hour_imbal <  TradingConfig::SESSION_DEAD_END_UTC);
        double imbal_thresh = TradingConfig::IMBALANCE_THRESHOLD *
                              (dead_imbal ? TradingConfig::DEAD_ZONE_IMBAL_MULT : 1.0);
        if (std::abs(imbalance) < imbal_thresh) {
            rejection_throttle_.record(key, "weak_imbalance");
            return false;
        }
        // Spot only  long side only (imbalance > 0 = bid pressure = buy signal)
        if (imbalance < 0) {
            rejection_throttle_.record(key, "short_not_supported_spot");
            return false;
        }

        enter(id, price, ts, s, LAYER_MICRO, true);
        return true;
    }

    // ======================================================================
    // LEAD-LAG  BTC leads ETH and SOL by 50-200ms
    // Latency requirement: < 35ms (LATENCY_LEADLAG_MAX_MS)
    //
    // MEASURED EDGE at our latency:
    //   WS latency p95 = 18-25ms
    //   BTCETH/SOL propagation = 50-200ms
    //   Remaining edge window = 25-175ms (conservative 75ms)
    //   TP = 14bp gross (+4bp net after 10bp costs)
    //   SL = 5bp
    //   Minimum win rate for positive EV = 73%
    //   (only fires on 12bp BTC move AND target hasn't moved 4bp yet)
    //
    // Only fires on ETH (id=1) and SOL (id=2)  BTC IS the leader
    // ======================================================================
    bool check_leadlag(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        // BTC does not follow itself
        if (id == 0) return false;

        const char* sym = sym_short(id);  // was hardcoded ETH/SOL  now works for all 7 symbols
        std::string key = std::string(sym) + " LEADLAG";

        if (ts < layer_lock_until_) {
            rejection_throttle_.record(key, "layer_locked");
            return false;
        }

        // LATENCY GATE: calibrated to leave enough edge window remaining
        if (latency_ms > TradingConfig::LATENCY_LEADLAG_MAX_MS) {
            rejection_throttle_.record(key, "latency_too_high");
            return false;
        }

        int direction = 0;
        if (!leadlag_.check_signal(id, latency_ms, direction)) {
            rejection_throttle_.record(key, "no_leadlag_signal");
            return false;
        }

        // Spot only  long side only
        if (direction < 0) {
            rejection_throttle_.record(key, "short_not_supported_spot");
            return false;
        }

        // SUSTAIN FILTER  reject fake BTC spikes that reverse immediately.
        // Require BTC move to still be >= 60% of threshold at entry time.
        // Real moves sustain. Fake moves are already reversing by the time
        // we check. Eliminates most SL hits on LEADLAG.
        double btc_now_bp = leadlag_.btc_move_bp();
        double sustain_threshold = TradingConfig::LEADLAG_BTC_THRESHOLD_BP * 0.6;
        if (std::fabs(btc_now_bp) < sustain_threshold) {
            rejection_throttle_.record(key, "btc_move_not_sustained");
            return false;
        }

        // Don't enter if already in a position on this symbol
        if (s.pos.state == POS_OPEN) return false;

        // ── CONFIRMATION GATE 1: Orderbook imbalance ──────────────────────
        // Require bid pressure > ask pressure before riding BTC lead move.
        // Filters setups where book is neutral or leaning short — those tend
        // to produce SL hits even when BTC move is real.
        const MarketTick& t = symbols_[id].last_tick;
        double ob_imbalance = 0.0;
        if (t.ask_size > 1e-9) {
            ob_imbalance = t.bid_size / t.ask_size;
        }
        if (ob_imbalance < TradingConfig::LEADLAG_CONFIRM_OB_RATIO) {
            rejection_throttle_.record(key, "ob_imbalance_weak");
            return false;
        }

        // ── CONFIRMATION GATE 2: Aggressive buy flow ──────────────────────
        // Require recent buy aggression to be elevated vs sell aggression.
        // Prevents entries when BTC moved but local order flow is net-sell.
        double flow_ratio = 0.0;
        if (symbols_[id].sell_vol_ema > 1e-9) {
            flow_ratio = symbols_[id].buy_vol_ema / symbols_[id].sell_vol_ema;
        }
        if (flow_ratio < TradingConfig::LEADLAG_CONFIRM_FLOW_RATIO) {
            rejection_throttle_.record(key, "flow_ratio_weak");
            return false;
        }

        std::printf("[LEADLAG] %s | btc_move=%.2fbp | sustain=%.2fbp | ob_imbal=%.2f | flow=%.2f | latency=%.1fms | ENTERING LONG\n",
                    sym, leadlag_.btc_move_bp(), btc_now_bp, ob_imbalance, flow_ratio, latency_ms);
        std::fflush(stdout);

        enter(id, price, ts, s, LAYER_LEADLAG, true);
        return true;
    }

    // -----------------------------------------------------------------------
    // try_liquidation_entry  spot long on short liquidation cascade
    // Called from on_tick when liq_engine_ has a pending valid signal
    // -----------------------------------------------------------------------
    bool try_liquidation_entry(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (id == 0) return false; // BTC too fast  liquidation already in price by the time we enter
        if (s.pos.state == POS_OPEN) return false;

        if (!liq_engine_.check_signal(id, price, ts, latency_ms)) return false;

        double notional = liq_engine_.get_notional(id);
        std::printf("[LIQ-ENTRY] %s | notional=$%.0f | price=%.4f | latency=%.1fms | ENTERING LONG\n",
            sym_short(id), notional, price, latency_ms);
        std::fflush(stdout);

        liq_engine_.consume_signal(id, ts);
        enter(id, price, ts, s, LAYER_LIQUIDATION, true);
        return true;
    }

    // -----------------------------------------------------------------------
    // try_funding_entry  spot long when perp funding deeply negative
    //
    // Signal: funding_rate < -0.0003 (-30bp/8h)
    //   Shorts on perp are paying longs. Shorts are crowded and overstretched.
    //   Spot gets sustained buy pressure as longs collect carry.
    //   This is a slow multi-hour move, not a scalp.
    //
    // Only BTC (id=0) and ETH (id=1). 4h cooldown. 2h max hold.
    // -----------------------------------------------------------------------
    bool try_funding_entry(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        // Only BTC and ETH  most reliable funding signal
        if (id != 0 && id != 1) return false;
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (!funding_ || !funding_->ready()) return false;

        // Cooldown  funding changes slowly, 4h between entries
        if (ts - s.last_funding_entry_ts < TradingConfig::FUNDING_SIG_COOLDOWN_MS) return false;

        double rate = funding_->rate();

        // Only enter on deeply negative funding (shorts crowded = longs have edge)
        if (rate >= TradingConfig::FUNDING_SIG_THRESHOLD) return false;

        // Latency not critical for this engine  it's a slow signal
        if (latency_ms > TradingConfig::FUNDING_SIG_LATENCY_MAX) return false;

        std::printf("[FUNDING-SIGNAL] %s | rate=%.5f%% (%.1fbp/8h) | ENTERING LONG\n",
            sym_short(id), rate * 100.0, rate * 10000.0);
        std::fflush(stdout);

        s.last_funding_entry_ts = ts;
        enter(id, price, ts, s, LAYER_FUNDING, true);
        return true;
    }

    // -----------------------------------------------------------------------
    // try_ngas_entry  spot long when NGAS drops sharply (risk-on rotation)
    //
    // Signal: NGASFetcher.signal_dir() == -1  (NGAS fell >2% in 15min)
    //   Natural Gas price drop = energy deflation = risk-on rotation into BTC/ETH
    //   Spot buys with wider TP/SL (macro signal, slow-burn, not latency-sensitive)
    //
    // Only BTC (id=0) and ETH (id=1). 8h cooldown. 1h max hold.
    // -----------------------------------------------------------------------
    bool try_ngas_entry(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (!ngas_) return false;
        if (id != 0 && id != 1) return false;
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        // Per-symbol cooldown (stored in SymbolState, not NGASEngine  consistent with funding)
        if (ts - s.last_ngas_entry_ts < TradingConfig::NGAS_COOLDOWN_MS) return false;

        // Latency gate (loose  not latency sensitive)
        if (latency_ms > TradingConfig::NGAS_LATENCY_MAX_MS) return false;

        // Arm baseline and check if signal is still valid
        ngas_->arm_if_new_signal(id, price);

        if (!ngas_->check_long_signal(id, price, ts, latency_ms)) return false;

        std::printf("[NGAS-ENTRY] %s | ngas_px=%.4f | ngas_chg=%.2f%% | crypto=%.4f | latency=%.1fms | ENTERING LONG\n",
            sym_short(id), ngas_->ngas_price(), ngas_->ngas_change_pct(), price, latency_ms);
        std::fflush(stdout);

        ngas_->consume_signal(id, ts);
        s.last_ngas_entry_ts = ts;
        enter(id, price, ts, s, LAYER_NGAS, true);
        return true;
    }

    // -----------------------------------------------------------------------
    // manage_pending  checks limit order fill status each tick
    // Called when pos.state == POS_PENDING
    // -----------------------------------------------------------------------
    void manage_pending(int id, double price, int64_t ts, SymbolState& s) {
        const MarketTick& t = s.last_tick;
        double ask = t.ask > 0.0 ? t.ask : price;
        double bid = t.bid > 0.0 ? t.bid : price;

        LimitStatus status = limit_orders_[id].update(ask, bid, ts);

        if (status == LimitStatus::FILLED) {
            // Limit filled  transition to open position
            double fill_px = limit_orders_[id].fill_price();
            s.pos.state      = POS_OPEN;
            s.pos.entry_price = fill_px;
            s.pos.entry_ts   = ts;
            s.pos.layer      = s.pos.pending_layer;
            s.pos.open_ticks = 0;
            s.pos.peak_price = fill_px;
            s.pos.mfe        = 0.0;
            s.pos.mae        = 0.0;

            const char* sym  = sym_short(id);
            const char* mode = (s.pos.layer == LAYER_MICRO)    ? "IMBAL"   :
                               (s.pos.layer == LAYER_LEADLAG)  ? "LEADLAG" :
                               (s.pos.layer == LAYER_VACUUM)        ? "VACUUM"    :
                               (s.pos.layer == LAYER_VWAP)          ? "VWAP"      :
                               (s.pos.layer == LAYER_LEADLAG_ETH_SOL)? "LL-ETH-SOL" :
                               (s.pos.layer == LAYER_IMPULSE)        ? "IMPULSE"   :
                               (s.pos.layer == LAYER_LIQUIDATION)  ? "LIQ"       :
                               (s.pos.layer == LAYER_FUNDING)       ? "FUND"      :
                               (s.pos.layer == LAYER_NGAS)          ? "NGAS"      :
                               (s.pos.layer == LAYER_ETH_LEAD)      ? "ETH-LEAD"  :
                               (s.pos.layer == LAYER_SOL_LEAD)      ? "SOL-LEAD"  :
                               (s.pos.layer == LAYER_VOLSHOCK)      ? "VOLSHOCK"  :
                               (s.pos.layer == LAYER_OFI)           ? "OFI"       :
                               (s.pos.layer == LAYER_SWEEP)         ? "SWEEP"     :
                               (s.pos.layer == LAYER_MM_PRESSURE)   ? "MM-PRESS"  : "EXPAND";

            std::printf("[MAKER-FILL] %s | %s | fill=%.4f | maker_cost=~4bp vs taker=~10bp\n",
                sym, mode, fill_px);
            std::fflush(stdout);

            // Capture entry context for shadow log
            s.entry_imbalance  = s.last_tick.book_imbalance;
            s.entry_flow_ratio = compute_flow_ratio(id);
            s.entry_spread_bps = s.last_tick.spread_bps;
            s.entry_btc_move   = leadlag_.btc_move_bp();
            s.entry_latency_ms = market_env_.latency_ms;

            limit_orders_[id].reset();

        } else if (status == LimitStatus::CANCELLED) {
            // Limit timed out or price moved away  abandon
            s.pos.state = POS_FLAT;
            s.pos.reset();
            limit_orders_[id].reset();
            open_positions_--;  // Undo the reserve from enter_pending
        }
        // PENDING: nothing to do, wait for next tick
    }

    void manage_position(int id, double price, int64_t ts, SymbolState& s) {
        // Increment hold time
        s.pos.open_ticks++;
        
        // Calculate current P&L in bp
        double move_bp = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
        
        // Update MFE (Maximum Favorable Excursion) and MAE (Maximum Adverse Excursion)
        s.pos.mfe = std::max(s.pos.mfe, move_bp);
        s.pos.mae = std::min(s.pos.mae, move_bp);
        
        // Scratch rule removed - was using /1000 (microseconds, not ms)
        // causing exit on tick 2 of every trade before price could move.
        // Hard stop (-15bp) + trailing stop + 30s timeout handle bad entries.
        
        // Update peak price for trailing
        if (move_bp > 0) {  // In profit
            s.pos.peak_price = std::max(s.pos.peak_price, price);
        }
        
        // VOLATILITY-NORMALIZED TRAILING EXIT
        // long_vol_ema is log-return stddev (e.g. 0.0003 = 0.03% per tick)
        // trail_distance must be in PRICE UNITS to compare with peak_price
        // Correct: trail_distance = multiplier * long_vol * current_price
        double long_vol = s.long_vol_ema;
        double trail_distance = TradingConfig::TRAIL_LONG_VOL_MULT * long_vol * price;
        
        // Calculate peak profit in bp
        double peak_profit_bp = (s.pos.peak_price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
        
        // Per-strategy TP/SL/timeout  all calibrated in TradingConfig
        // against the 10bp round-trip cost at our measured Tokyo latency
        double tp_bp      = 0.0;
        double sl_bp      = 0.0;
        int64_t max_hold  = 0;

        if (s.pos.layer == LAYER_LIQUIDATION) {
            // Liquidation cascade: short liq on perp  spot follows up
            // TP=12bp, SL=4bp, hold 5s
            tp_bp     = TradingConfig::LIQ_TP_BP;
            sl_bp     = TradingConfig::LIQ_SL_BP;
            max_hold  = TradingConfig::LIQ_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_FUNDING) {
            // Funding signal: deeply negative funding = sustained long
            // Wide TP/SL  slow-burn multi-hour move
            tp_bp     = TradingConfig::FUNDING_SIG_TP_BP;
            sl_bp     = TradingConfig::FUNDING_SIG_SL_BP;
            max_hold  = TradingConfig::FUNDING_SIG_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_NGAS) {
            // NGAS lead-lag: macro risk-on rotation signal
            // Wide TP/SL  macro noise is larger than microstructure
            tp_bp     = TradingConfig::NGAS_TP_BP;
            sl_bp     = TradingConfig::NGAS_SL_BP;
            max_hold  = TradingConfig::NGAS_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_LEADLAG) {
            // TP=8bp ceiling. SL=3bp. Hold 5s max.
            // TRAIL FLOOR: arm at 3bp profit, lock in 75% of peak.
            // Rationale: 90% of trades exit TIMEOUT. avg MFE=5.9bp, avg giveback=0.9bp.
            // Worst case: ETH MFE=7.6bp → exit=0.16bp (gave back 7.4bp).
            // Trail floor: exit immediately if price drops below 75% of MFE peak.
            tp_bp     = TradingConfig::LEADLAG_TP_BP;
            sl_bp     = TradingConfig::LEADLAG_SL_BP;
            max_hold  = TradingConfig::LEADLAG_MAX_HOLD_MS;
            if (s.pos.mfe >= 3.0 && move_bp < s.pos.mfe * 0.75) {
                std::printf("[LEADLAG-TRAIL] %s | peak=%.2fbp floor=%.2fbp now=%.2fbp\n",
                    sym_short(id), s.pos.mfe, s.pos.mfe * 0.75, move_bp);
                std::fflush(stdout);
                pending_exit_reason_ = "TRAIL_FLOOR";
                exit(id, move_bp, ts, s);
                return;
            }
        } else if (s.pos.layer == LAYER_LEADLAG_ETH_SOL) {
            // ETHSOL: TP=12bp, SL=5bp, hold 2.5s. Same trail floor as LEADLAG.
            tp_bp     = TradingConfig::LEADLAG_ETH_SOL_TP_BP;
            sl_bp     = TradingConfig::LEADLAG_ETH_SOL_SL_BP;
            max_hold  = TradingConfig::LEADLAG_ETH_SOL_MAX_HOLD_MS;
            if (s.pos.mfe >= 3.0 && move_bp < s.pos.mfe * 0.75) {
                std::printf("[LL-ETH-SOL-TRAIL] %s | peak=%.2fbp floor=%.2fbp now=%.2fbp\n",
                    sym_short(id), s.pos.mfe, s.pos.mfe * 0.75, move_bp);
                std::fflush(stdout);
                pending_exit_reason_ = "TRAIL_FLOOR";
                exit(id, move_bp, ts, s);
                return;
            }
        } else if (s.pos.layer == LAYER_MICRO) {
            // TP=12bp gross  +2bp net. SL=3bp. Hold 8s max.
            tp_bp     = TradingConfig::IMBALANCE_TP_BP;
            sl_bp     = TradingConfig::IMBALANCE_SL_BP;
            max_hold  = TradingConfig::IMBALANCE_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_VACUUM) {
            // TP=16bp gross  +6bp net. SL=6bp. Hold 12s max.
            tp_bp     = TradingConfig::VACUUM_TP_BP;
            sl_bp     = TradingConfig::VACUUM_SL_BP;
            max_hold  = TradingConfig::VACUUM_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_VWAP) {
            // TP=18bp gross  +8bp net. SL=7bp. Hold 45s max (slower reversion).
            tp_bp     = TradingConfig::VWAP_TP_BP;
            sl_bp     = TradingConfig::VWAP_SL_BP;
            max_hold  = TradingConfig::VWAP_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_EXPANSION) {
            // EXPANSION: own tighter params. Cut losers fast at 12s/5bp SL.
            // LINK(5)/AVAX(4)/POL(6): thin books, moves run 11-22bp -- 6bp TP cuts winners short
            bool is_alt_expand = (id == 4 || id == 5 || id == 6);
            tp_bp    = is_alt_expand ? TradingConfig::EXPANSION_ALT_TP_BP : TradingConfig::EXPANSION_TP_BP;
            sl_bp    = TradingConfig::EXPANSION_SL_BP;
            max_hold = TradingConfig::EXPANSION_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_ETH_LEAD) {
            tp_bp    = TradingConfig::ETH_LEAD_TP_BP;
            sl_bp    = TradingConfig::ETH_LEAD_SL_BP;
            max_hold = TradingConfig::ETH_LEAD_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_SOL_LEAD) {
            tp_bp    = TradingConfig::SOL_LEAD_TP_BP;
            sl_bp    = TradingConfig::SOL_LEAD_SL_BP;
            max_hold = TradingConfig::SOL_LEAD_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_VOLSHOCK) {
            tp_bp    = TradingConfig::VOLSHOCK_TP_BP;
            sl_bp    = TradingConfig::VOLSHOCK_SL_BP;
            max_hold = TradingConfig::VOLSHOCK_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_OFI) {
            tp_bp    = TradingConfig::OFI_TP_BP;
            sl_bp    = TradingConfig::OFI_SL_BP;
            max_hold = TradingConfig::OFI_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_SWEEP) {
            tp_bp    = TradingConfig::SWEEP_TP_BP;
            sl_bp    = TradingConfig::SWEEP_SL_BP;
            max_hold = TradingConfig::SWEEP_MAX_HOLD_MS;
        } else if (s.pos.layer == LAYER_MM_PRESSURE) {
            tp_bp    = TradingConfig::MM_TP_BP;
            sl_bp    = TradingConfig::MM_SL_BP;
            max_hold = TradingConfig::MM_MAX_HOLD_MS;
        } else {
            // IMPULSE: alt coins (AVAX/LINK/POL) get wider TP -- thin books, moves run to 20bp+
            // SOL: standard TP (moves are shallower than micro-caps)
            bool is_alt_impulse = (id == 4 || id == 5 || id == 6);
            tp_bp     = is_alt_impulse ? TradingConfig::IMPULSE_ALT_TP_BP : TradingConfig::IMPULSE_TP_BP;
            sl_bp     = TradingConfig::IMPULSE_SL_BP;
            max_hold  = TradingConfig::IMPULSE_MAX_HOLD_MS;
        }

        if (executor_ && executor_->is_shadow()) {
            // SHADOW scalper profile: faster close cycle and smaller targets for
            // high sample throughput during tuning.
            tp_bp = std::max(2.0, tp_bp * 0.55);
            sl_bp = std::max(2.5, sl_bp * 0.80);
            max_hold = std::min<int64_t>(max_hold, 2500);
        }

        // Hard take-profit
        if (move_bp >= tp_bp) {
            std::printf("[TP-HIT] %s | layer=%s | move=%.2fbp >= tp=%.2fbp\n",
                sym_short(id),
                (s.pos.layer == LAYER_MICRO)   ? "IMBAL"   :
                (s.pos.layer == LAYER_LEADLAG)  ? "LEADLAG" :
                (s.pos.layer == LAYER_VACUUM)        ? "VACUUM"    :
                (s.pos.layer == LAYER_VWAP)          ? "VWAP"      :
                (s.pos.layer == LAYER_LEADLAG_ETH_SOL)? "LL-ETH-SOL" : "IMPULSE",
                move_bp, tp_bp);
            std::fflush(stdout);
            pending_exit_reason_ = "TP";
            exit(id, move_bp, ts, s);
            return;
        }

        // Hard stop loss
        if (move_bp <= -sl_bp) {
            std::printf("[SL-HIT] %s | move=%.2fbp <= -%.2fbp\n",
                sym_short(id), move_bp, sl_bp);
            std::fflush(stdout);
            pending_exit_reason_ = "SL";
            exit(id, move_bp, ts, s);
            return;
        }

        // Minimum hold: don't exit before MIN_HOLD_TICKS
        if (s.pos.open_ticks < TradingConfig::MIN_HOLD_TICKS) {
            return;
        }

        // Breakeven protection for EXPANSION  if trade peaked at 2bp profit, floor at entry
        // For LONG: move_bp positive = profit. For SHORT: move_bp negative = profit.
        // peak_profit_bp = max favorable move regardless of direction.
        double expand_peak_profit = s.pos.is_long ? s.pos.mfe : (-s.pos.mae);
        if (s.pos.layer == LAYER_EXPANSION && expand_peak_profit >= 2.0) {
            // Current profit has reversed back to 0 or below  exit at near-breakeven
            double current_profit = s.pos.is_long ? move_bp : (-move_bp);
            if (current_profit <= 0.0) {
                std::printf("[BREAKEVEN] %s | EXPAND | peak=%.2fbp now=%.2fbp  protecting gains\n",
                    sym_short(id), expand_peak_profit, current_profit);
                std::fflush(stdout);
                pending_exit_reason_ = "TRAIL";  // counts as trail exit in stats
                exit(id, move_bp, ts, s);
                return;
            }
        }

        // Trailing stop for IMPULSE/EXPAND (once in profit > cost floor)
        if (s.pos.layer == LAYER_IMPULSE || s.pos.layer == LAYER_EXPANSION) {
            if (peak_profit_bp >= TradingConfig::MIN_PROFIT_TO_TRAIL_BP) {
                if (price < s.pos.peak_price - trail_distance) {
                    std::printf("[TRAIL-STOP] %s | peak=%.2fbp trail_dist=%.4f move=%.2fbp\n",
                        sym_short(id),
                        peak_profit_bp, trail_distance / price * 10000.0, move_bp);
                    std::fflush(stdout);
                    pending_exit_reason_ = "TRAIL";
                    exit(id, move_bp, ts, s);
                    return;
                }
            }
        }

        // Time-based forced exit
        if (ts - s.pos.entry_ts > max_hold) {
            std::printf("[MAX-HOLD] %s | hold=%ldms > %ldms\n",
                sym_short(id),
                (long)(ts - s.pos.entry_ts), (long)max_hold);
            std::fflush(stdout);
            pending_exit_reason_ = "TIMEOUT";
            exit(id, move_bp, ts, s);
            return;
        }
    }

    // ======================================================================
    // LEAD-LAG ETH  SOL
    // ETH leads SOL by ~30-80ms (smaller correlation window than BTCETH/SOL)
    // Only fires on SOL (id=2). Long-only, spot-valid.
    // ETH needs 6bp move; SOL must not have moved 3bp yet.
    // TP=12bp, SL=5bp  slightly tighter than BTCETH/SOL due to smaller edge
    // ======================================================================
    bool check_leadlag_eth_sol(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        // Only SOL follows ETH
        if (id != 2) return false;

        const char* sym = "SOL";
        std::string key = "SOL LL-ETH-SOL";
        (void)sym;  // used in printf below

        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        if (ts < layer_lock_until_) {
            rejection_throttle_.record(key, "layer_locked");
            return false;
        }
        // ETHSOL window is tighter than BTCETH  use imbalance limit (20ms)
        if (latency_ms > TradingConfig::LATENCY_IMBALANCE_MAX_MS) {
            rejection_throttle_.record(key, "latency_too_high");
            return false;
        }

        int direction = 0;
        if (!leadlag_.check_signal_eth_sol(latency_ms, direction)) {
            rejection_throttle_.record(key, "no_eth_sol_signal");
            return false;
        }
        // Long-only spot
        if (direction < 0) {
            rejection_throttle_.record(key, "spot_long_only");
            return false;
        }

        // ORDER FLOW CONFIRMATION  ETH led, but SOL must also have buy pressure
        // If SOL order flow is weak, ETH move is already absorbed, skip entry
        double flow = compute_flow_ratio(id);
        if (flow < TradingConfig::FLOW_CONFIRM_THRESHOLD) {
            rejection_throttle_.record(key, "weak_sol_flow");
            return false;
        }

        std::printf("[LL-ETH-SOL] SOL | eth_move=%.2fbp | flow=%.2f | latency=%.1fms | ENTERING LONG\n",
                    leadlag_.eth_move_bp(), flow, latency_ms);
        std::fflush(stdout);

        enter(id, price, ts, s, LAYER_LEADLAG_ETH_SOL, true);
        return true;
    }
    // ======================================================================
    // TIER 2: ETH -> SOL/BNB/AVAX/LINK/POL lead-lag
    // Same edge mechanism as BTC->alts but ETH as leader.
    // ETH moves propagate to alts in 30-100ms.
    // Threshold 10bp (ETH moves slower than BTC, need bigger signal).
    // ======================================================================
    bool check_eth_lead(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        // ETH (id=1) cannot follow itself, BTC cannot follow ETH
        if (id <= 1) return false;
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (ts < layer_lock_until_) return false;
        if (latency_ms > TradingConfig::LATENCY_LEADLAG_MAX_MS) return false;

        int direction = 0;
        if (!leadlag_.check_signal_eth_lead(id, latency_ms, direction)) return false;
        if (direction < 0) return false; // long only

        // Sustain filter -- ETH move must still be >= 60% of threshold
        double eth_now_bp = leadlag_.eth_move_bp();
        double sustain = TradingConfig::LEADLAG_ETH_SOL_THRESHOLD_BP * 0.6;
        if (std::fabs(eth_now_bp) < sustain) return false;

        std::printf("[ETH-LEAD] %s | eth_move=%.2fbp | sustain=%.2fbp | latency=%.1fms | ENTERING LONG\n",
                    sym_short(id), eth_now_bp, sustain, latency_ms);
        std::fflush(stdout);

        enter(id, price, ts, s, LAYER_ETH_LEAD, true);
        return true;
    }

    // ======================================================================
    // TIER 3: SOL -> AVAX/POL lead-lag
    // SOL is the fastest L1 and leads AVAX and POL by 20-60ms.
    // Threshold 12bp (SOL is volatile, need stronger signal).
    // ======================================================================
    bool check_sol_lead(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        // Only AVAX (4) and POL (6) follow SOL
        if (id != 4 && id != 6) return false;
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (ts < layer_lock_until_) return false;
        if (latency_ms > TradingConfig::LATENCY_LEADLAG_MAX_MS) return false;

        int direction = 0;
        if (!leadlag_.check_signal_sol_lead(id, latency_ms, direction)) return false;
        if (direction < 0) return false; // long only

        // Sustain filter
        double sol_now_bp = leadlag_.sol_move_bp();
        double sustain = 7.0; // 12bp threshold * 0.6
        if (std::fabs(sol_now_bp) < sustain) return false;

        std::printf("[SOL-LEAD] %s | sol_move=%.2fbp | latency=%.1fms | ENTERING LONG\n",
                    sym_short(id), sol_now_bp, latency_ms);
        std::fflush(stdout);

        enter(id, price, ts, s, LAYER_SOL_LEAD, true);
        return true;
    }

    // ======================================================================
    // VOLUME SHOCK CONTINUATION
    // Volume spike (3x+ baseline) + price displacement (8bp+) = continuation.
    // Requires both volume AND price to confirm -- filters EXPAND noise.
    // Conservative params: TP=10bp, SL=4bp, hold=6s, 8s cooldown.
    // ======================================================================
    bool check_vol_shock(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (latency_ms > TradingConfig::LATENCY_LEADLAG_MAX_MS) return false;

        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0) return false;
        if (t.spread_bps > VolumeShockEngine::MAX_SPREAD_BPS) return false;

        double volume = t.bid_size + t.ask_size;
        int direction = 0;
        if (!vol_shock_.on_tick(id, price, volume, t.spread_bps, ts, direction)) return false;

        std::printf("[VOLSHOCK] %s | vol_ratio=%.2f | price=%.4f | latency=%.1fms | ENTERING LONG\n",
                    sym_short(id), vol_shock_.get_vol_ratio(id), price, latency_ms);
        std::fflush(stdout);

        enter(id, price, ts, s, LAYER_VOLSHOCK, true);
        return true;
    }

        // Edge: ask-side depth drains >40% vs EMA baseline without price moving.
    // When the ask wall disappears, price gaps up through the vacuum.
    // Fires in GRIND or BUILDUP  best in calmer regimes where a sudden
    // ask drain is structural, not just noise.
    // EV: TP=16bp gross (+6bp net), SL=6bp, win rate target ~65%
    // ======================================================================
    bool check_vacuum(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = sym_short(id);
        std::string key = std::string(sym) + " VACUUM";

        // Per-symbol guard
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        if (latency_ms > TradingConfig::LATENCY_VACUUM_MAX_MS) {
            rejection_throttle_.record(key, "latency_too_high");
            return false;
        }
        // Only fires in GRIND or BUILDUP  BREAKOUT has own engines
        if (s.regime == REGIME_DEAD || s.regime == REGIME_BREAKOUT) {
            rejection_throttle_.record(key, "wrong_regime");
            return false;
        }
        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0 || t.ask_size <= 0.0) {
            rejection_throttle_.record(key, "no_book_data");
            return false;
        }
        if (t.spread_bps > TradingConfig::VACUUM_MAX_SPREAD_BPS) {
            rejection_throttle_.record(key, "spread_wide");
            return false;
        }
        // Need established ask depth baseline
        if (!s.ask_depth_init || s.ask_depth_ema <= 0.0) {
            rejection_throttle_.record(key, "no_depth_baseline");
            return false;
        }
        // Core signal: current ask depth is less than (1 - drain_ratio) * baseline
        double drain_threshold = s.ask_depth_ema * (1.0 - TradingConfig::VACUUM_ASK_DRAIN_RATIO);
        if (t.ask_size >= drain_threshold) {
            rejection_throttle_.record(key, "ask_not_drained");
            return false;
        }
        // Bid must still be present  confirms buyers are active, not just thin market
        if (t.book_imbalance < TradingConfig::VACUUM_MIN_IMBALANCE) {
            rejection_throttle_.record(key, "no_bid_confirmation");
            return false;
        }
        std::printf("[VACUUM] %s | ask_size=%.2f vs baseline=%.2f (drain=%.0f%%) | imbal=%.2f | ENTERING LONG\n",
                    sym, t.ask_size, s.ask_depth_ema,
                    (1.0 - t.ask_size / s.ask_depth_ema) * 100.0,
                    t.book_imbalance);
        std::fflush(stdout);
        enter(id, price, ts, s, LAYER_VACUUM, true);
        return true;
    }

    // ======================================================================
    // VWAP REVERSION  spot-only long
    // Edge: in GRIND regime, price pulls >20bp below session VWAP with
    // positive book imbalance = buy the mean reversion back toward VWAP.
    // Classic institutional anchor  large players accumulate at VWAP
    // discounts, pulling price back. Very high win rate in ranging markets.
    // Not valid in BREAKOUT (trending away from VWAP is a feature, not a bug).
    // EV: TP=18bp gross (+8bp net), SL=7bp, win rate target ~68%
    // ======================================================================
    bool check_vwap_reversion(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = sym_short(id);
        std::string key = std::string(sym) + " VWAP";

        // Per-symbol guard
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;

        if (latency_ms > TradingConfig::LATENCY_VWAP_MAX_MS) {
            rejection_throttle_.record(key, "latency_too_high");
            return false;
        }
        // Only fires in GRIND  VWAP reversion fails in trending regimes
        if (s.regime != REGIME_GRIND) {
            rejection_throttle_.record(key, "not_grind");
            return false;
        }
        // Need established VWAP
        if (s.session_vwap <= 0.0 || s.vwap_cum_vol < 10.0) {
            rejection_throttle_.record(key, "vwap_not_ready");
            return false;
        }
        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0) {
            rejection_throttle_.record(key, "no_book_data");
            return false;
        }
        if (t.spread_bps > TradingConfig::VWAP_MAX_SPREAD_BPS) {
            rejection_throttle_.record(key, "spread_wide");
            return false;
        }
        // Core signal: price is below VWAP by entry deviation threshold
        double deviation_bp = (s.session_vwap - price) / s.session_vwap * 10000.0;
        if (deviation_bp < TradingConfig::VWAP_ENTRY_DEVIATION_BP) {
            rejection_throttle_.record(key, "not_far_enough_below_vwap");
            return false;
        }
        // Don't enter if too far below VWAP  that's a breakdown, not a dip
        if (deviation_bp > TradingConfig::VWAP_MAX_DEVIATION_BP) {
            rejection_throttle_.record(key, "too_far_below_vwap");
            return false;
        }
        // Book must show bid pressure  buyers are stepping in
        if (t.book_imbalance < TradingConfig::VWAP_MIN_IMBALANCE) {
            rejection_throttle_.record(key, "no_bid_confirmation");
            return false;
        }
        std::printf("[VWAP-REV] %s | price=%.4f | vwap=%.4f | dev=%.1fbp | imbal=%.2f | ENTERING LONG\n",
                    sym, price, s.session_vwap, deviation_bp, t.book_imbalance);
        std::fflush(stdout);
        enter(id, price, ts, s, LAYER_VWAP, true);
        return true;
    }

    // =========================================================================
    // LAYER_OFI  Order Flow Imbalance Pressure
    // =========================================================================
    // Edge: when the 33-tick OFI ratio is strongly positive AND a volume spike
    // confirms buy aggression, price tends to follow 10-40 bps within seconds.
    // Fires in GRIND or BUILDUP only (BREAKOUT has its own momentum engines).
    // Maker entry — the edge is structural, not latency-sensitive.
    // EV at 45% WR: 0.45*18 - 0.55*6 = 8.1 - 3.3 = +4.8bp net (after ~4bp maker cost)
    // =========================================================================
    bool check_ofi_pressure(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (latency_ms > TradingConfig::LATENCY_IMBALANCE_MAX_MS) return false;

        // Regime: GRIND or BUILDUP only
        if (s.regime == REGIME_DEAD || s.regime == REGIME_BREAKOUT) {
            rejection_throttle_.record(std::string(sym_short(id)) + " OFI", "wrong_regime");
            return false;
        }
        if (!s.ofi_ema_init) return false;

        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0) return false;
        if (t.spread_bps > TradingConfig::OFI_MAX_SPREAD_BPS) {
            rejection_throttle_.record(std::string(sym_short(id)) + " OFI", "spread_wide");
            return false;
        }

        double total_flow = s.ofi_buy_ema + s.ofi_sell_ema;
        if (total_flow < 1e-9) return false;
        double ofi_ratio = (s.ofi_buy_ema - s.ofi_sell_ema) / total_flow;

        // Must exceed threshold in the long direction only (spot = long bias)
        if (ofi_ratio < TradingConfig::OFI_RATIO_THRESHOLD) {
            rejection_throttle_.record(std::string(sym_short(id)) + " OFI", "ofi_weak");
            return false;
        }

        // Volume confirmation: current trade size must be above EMA baseline
        if (!s.trade_size_init || s.trade_size_ema < 1e-9) {
            rejection_throttle_.record(std::string(sym_short(id)) + " OFI", "no_vol_baseline");
            return false;
        }
        double vol_ratio = (t.agg_buy_volume + t.agg_sell_volume) / s.trade_size_ema;
        if (vol_ratio < TradingConfig::OFI_VOLUME_SPIKE_MULT) {
            rejection_throttle_.record(std::string(sym_short(id)) + " OFI", "volume_low");
            return false;
        }

        // Book must also lean long (bid depth >= ask depth)
        if (t.book_imbalance < TradingConfig::OFI_BOOK_CONFIRM_IMBAL) {
            rejection_throttle_.record(std::string(sym_short(id)) + " OFI", "book_neutral");
            return false;
        }

        std::printf("[OFI] %s | ofi=%.3f | vol_spike=%.2fx | book=%.3f | lat=%.1fms | LONG\n",
                    sym_short(id), ofi_ratio, vol_ratio, t.book_imbalance, latency_ms);
        std::fflush(stdout);
        enter(id, price, ts, s, LAYER_OFI, true);
        return true;
    }

    // =========================================================================
    // LAYER_SWEEP  Liquidity Sweep / Stop Run
    // =========================================================================
    // Edge: a spike in aggressive trade size (>5x EMA) combined with a depth
    // collapse on the same side signals a stop run.  We trade momentum continuation
    // (the most reliable outcome — 60-70% of sweeps continue 20-80 bps).
    // Taker entry only — sweep edges decay in <200ms, maker fill is uncertain.
    // EV at 55% WR: 0.55*22 - 0.45*8 = 12.1 - 3.6 = +8.5bp net (after ~8bp taker)
    // =========================================================================
    bool check_sweep(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (latency_ms > TradingConfig::LATENCY_LEADLAG_MAX_MS) return false;

        // Sweeps work in any regime — they are self-creating events
        if (s.regime == REGIME_DEAD) {
            rejection_throttle_.record(std::string(sym_short(id)) + " SWEEP", "dead_regime");
            return false;
        }
        if (!s.trade_size_init || s.trade_size_ema < 1e-9) return false;

        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0) return false;
        if (t.spread_bps > TradingConfig::SWEEP_MAX_SPREAD_BPS) {
            rejection_throttle_.record(std::string(sym_short(id)) + " SWEEP", "spread_wide");
            return false;
        }

        double trade_size = t.agg_buy_volume + t.agg_sell_volume;
        double size_spike  = trade_size / s.trade_size_ema;

        // Condition 1: aggressive trade spike
        if (size_spike < TradingConfig::SWEEP_SIZE_SPIKE_MULT) {
            rejection_throttle_.record(std::string(sym_short(id)) + " SWEEP", "size_spike_low");
            return false;
        }

        // Condition 2: ask-side depth collapse from previous tick (long sweep signal)
        // bid-side collapse would be a short, but we are spot-only (long bias)
        if (s.prev_ask_depth < 1e-9) return false;
        double ask_collapse = (s.prev_ask_depth - t.ask_size) / s.prev_ask_depth;

        if (ask_collapse < TradingConfig::SWEEP_DEPTH_COLLAPSE_RATIO) {
            rejection_throttle_.record(std::string(sym_short(id)) + " SWEEP", "depth_intact");
            return false;
        }

        // Condition 3: trade must be buyer-initiated (momentum direction confirmed)
        if (t.agg_buy_volume <= t.agg_sell_volume) {
            rejection_throttle_.record(std::string(sym_short(id)) + " SWEEP", "no_buy_aggression");
            return false;
        }

        std::printf("[SWEEP] %s | size_spike=%.1fx | ask_collapse=%.1f%% | lat=%.1fms | LONG\n",
                    sym_short(id), size_spike, ask_collapse * 100.0, latency_ms);
        std::fflush(stdout);
        // Sweeps use taker entry — edge window is <200ms
        enter(id, price, ts, s, LAYER_SWEEP, true);
        return true;
    }

    // =========================================================================
    // LAYER_MM_PRESSURE  Market Maker Inventory Pressure
    // =========================================================================
    // Edge: when MMs have accumulated inventory on one side they are forced to
    // rebalance, creating a slow but directional drift.  Signals:
    //   - persistent book imbalance (slow EMA > threshold)
    //   - cumulative price drift in the same direction over 100 ticks
    //   - trades being absorbed (book imbalance sustained despite flow)
    // Maker entry — this is a slow structural move, not a latency play.
    // EV at 50% WR: 0.50*20 - 0.50*7 = 10 - 3.5 = +6.5bp net (after ~4bp maker)
    // =========================================================================
    bool check_mm_pressure(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (s.pos.state == POS_OPEN || s.pos.state == POS_PENDING) return false;
        if (latency_ms > TradingConfig::LATENCY_IMBALANCE_MAX_MS) return false;

        // Best in GRIND — MM rebalancing is a ranging-market phenomenon
        if (s.regime != REGIME_GRIND) {
            rejection_throttle_.record(std::string(sym_short(id)) + " MM", "not_grind");
            return false;
        }
        if (!s.mm_imbal_init || s.mm_drift_ticks < 20) return false;

        const MarketTick& t = s.last_tick;
        if (t.bid <= 0.0 || t.ask <= 0.0) return false;
        if (t.spread_bps > TradingConfig::MM_MAX_SPREAD_BPS) {
            rejection_throttle_.record(std::string(sym_short(id)) + " MM", "spread_wide");
            return false;
        }

        // Condition 1: slow book imbalance EMA must be positive (bid-heavy)
        if (s.mm_imbal_ema < TradingConfig::MM_IMBAL_EMA_THRESHOLD) {
            rejection_throttle_.record(std::string(sym_short(id)) + " MM", "imbal_ema_low");
            return false;
        }

        // Condition 2: cumulative price drift must be positive (upward pressure)
        // Normalise drift as bps of current price
        double drift_bps = (s.mm_drift_sum / price) * 10000.0;
        if (drift_bps < TradingConfig::MM_DRIFT_BPS_THRESHOLD) {
            rejection_throttle_.record(std::string(sym_short(id)) + " MM", "drift_weak");
            return false;
        }

        // Condition 3: OFI must also be buy-sided (absorption confirmation)
        if (s.ofi_ema_init) {
            double total = s.ofi_buy_ema + s.ofi_sell_ema;
            if (total > 1e-9 && (s.ofi_buy_ema - s.ofi_sell_ema) / total < 0.05) {
                rejection_throttle_.record(std::string(sym_short(id)) + " MM", "ofi_not_confirming");
                return false;
            }
        }

        std::printf("[MM-PRESSURE] %s | imbal_ema=%.3f | drift=%.2fbp | ticks=%d | lat=%.1fms | LONG\n",
                    sym_short(id), s.mm_imbal_ema, drift_bps, s.mm_drift_ticks, latency_ms);
        std::fflush(stdout);
        enter(id, price, ts, s, LAYER_MM_PRESSURE, true);
        return true;
    }

    void enter(int id, double price, int64_t ts, SymbolState& s, LayerMode layer, bool is_long = true) {
        Signal sig;
        sig.symbol = sym_full(id);
        sig.layer = (layer == LAYER_IMPULSE) ? LayerType::IMPULSE :
                    (layer == LAYER_EXPANSION) ? LayerType::EXPAND :
                    (layer == LAYER_ETH_LEAD)  ? LayerType::EXPAND :
                    (layer == LAYER_SOL_LEAD)  ? LayerType::EXPAND :
                    (layer == LAYER_VOLSHOCK)   ? LayerType::EXPAND :
                    (layer == LAYER_OFI)        ? LayerType::EXPAND :
                    (layer == LAYER_SWEEP)      ? LayerType::EXPAND :
                    (layer == LAYER_MM_PRESSURE)? LayerType::EXPAND :
                    (layer == LAYER_MICRO) ? LayerType::MICRO : LayerType::LEADLAG;
        // Expected gross edge per layer (gross TP targets from TradingConfig)
        sig.expected_bps = (layer == LAYER_LIQUIDATION) ? TradingConfig::LIQ_TP_BP :
                           (layer == LAYER_FUNDING)     ? TradingConfig::FUNDING_SIG_TP_BP :
                           (layer == LAYER_NGAS)         ? TradingConfig::NGAS_TP_BP :
                           (layer == LAYER_LEADLAG) ? TradingConfig::LEADLAG_TP_BP :
                           (layer == LAYER_MICRO)   ? TradingConfig::IMBALANCE_TP_BP :
                           (layer == LAYER_VACUUM)          ? TradingConfig::VACUUM_TP_BP :
                           (layer == LAYER_VWAP)              ? TradingConfig::VWAP_TP_BP :
                           (layer == LAYER_LEADLAG_ETH_SOL)   ? TradingConfig::LEADLAG_ETH_SOL_TP_BP :
                           (layer == LAYER_IMPULSE)            ? TradingConfig::IMPULSE_TP_BP :
                                                                 TradingConfig::IMPULSE_TP_BP;
        sig.confidence = 1.0;

        // HARD LATENCY BACKSTOP  never enter on stale data regardless of engine
        if (market_env_.latency_ms > TradingConfig::LATENCY_HARD_LIMIT_MS) {
            std::string key = std::string(sym_short(id)) + " ENTER";
            rejection_throttle_.record(key, "latency_hard_block");
            return;
        }

        // COST FLOOR GATE — per-layer, based on actual execution model
        // TAKER layers (post at ask, guaranteed fill): 8bp round trip cost → 12bp floor
        //   LEADLAG, IMPULSE, ETH_LEAD, SOL_LEAD, VOLSHOCK, LIQUIDATION
        // MAKER layers (post below mid, rebate): ~4bp cost → 4bp floor
        //   IMBALANCE, EXPANSION, VWAP, VACUUM
        // LEADLAG/LL-ETH-SOL now use aggressive maker entry — cost ~4bp, floor = MAKER
        // Pure taker layers (IMPULSE etc disabled but kept for correctness if re-enabled)
        bool is_taker_layer = (layer == LAYER_IMPULSE        ||
                               layer == LAYER_ETH_LEAD       ||
                               layer == LAYER_SOL_LEAD       ||
                               layer == LAYER_VOLSHOCK       ||
                               layer == LAYER_LIQUIDATION ||
                               layer == LAYER_SWEEP);
        bool is_leadlag_layer = (layer == LAYER_LEADLAG || layer == LAYER_LEADLAG_ETH_SOL);
        double cost_floor = is_taker_layer  ? TradingConfig::COST_FLOOR_BP          // 12bp — taker
                          : is_leadlag_layer ? TradingConfig::MAKER_COST_FLOOR_BP    // 4bp  — aggressive maker
                          : (layer == LAYER_EXPANSION)
                              ? TradingConfig::EXPANSION_COST_FLOOR_BP               // 8bp  — disabled but correct
                              : TradingConfig::MAKER_COST_FLOOR_BP;                  // 4bp  — IMBALANCE/VWAP/VACUUM
        if (sig.expected_bps < cost_floor) {
            std::string key = std::string(sym_short(id)) +
                             " " + ((layer == LAYER_LEADLAG)  ? "LEADLAG" :
                                    (layer == LAYER_IMPULSE)   ? "IMPULSE" :
                                    (layer == LAYER_EXPANSION) ? "EXPAND"  :
                                    (layer == LAYER_VACUUM)          ? "VACUUM"    :
                                    (layer == LAYER_VWAP)              ? "VWAP"      :
                                    (layer == LAYER_LEADLAG_ETH_SOL)   ? "LL-ETH-SOL" : "IMBAL");
            rejection_throttle_.record(key, "cost_floor");
            return;
        }
        
        if (!governor_.approve(sig)) {
            return;
        }
        
        // PHASE 2: Compute size using full allocation stack
        double base_weight = (layer == LAYER_IMPULSE || layer == LAYER_EXPANSION) ?
                            adaptive_allocator_.impulse_weight() :
                            adaptive_allocator_.maker_weight();
        
        // Apply microstructure biases
        double micro_bias = (layer == LAYER_IMPULSE || layer == LAYER_EXPANSION) ?
                           micro_edge_[id].impulse_bias() :
                           micro_edge_[id].maker_bias();
        
        // Apply regime multipliers
        double regime_mult = (layer == LAYER_IMPULSE || layer == LAYER_EXPANSION) ?
                            regime_classifiers_[id].impulse_multiplier() :
                            regime_classifiers_[id].maker_multiplier();
        
        // Apply toxic flow adjustments
        if (toxic_flow_[id].toxic_regime()) {
            if (layer == LAYER_IMPULSE || layer == LAYER_EXPANSION) {
                regime_mult *= toxic_flow_[id].impulse_boost();
            } else {
                regime_mult *= toxic_flow_[id].maker_suppression();
            }
        }
        
        double final_weight = base_weight * micro_bias * regime_mult;
        
        // Compute final size via CapitalControlLayer  use real live values
        CapitalControlLayer::MarketEnv cap_env;
        cap_env.short_range    = market_env_.short_range;
        cap_env.long_range     = market_env_.long_range;
        cap_env.spread_bps     = market_env_.spread_bps;          // real tick spread
        cap_env.book_imbalance = market_env_.book_imbalance;      // real book pressure
        // queue_density: normalise bid+ask depth vs recent baseline
        {
            const MarketTick& lt = symbols_[id].last_tick;
            double depth = lt.bid_size + lt.ask_size;
            cap_env.queue_density = std::min(1.0, depth / std::max(depth_baseline_[id], 1e-6));
        }
        cap_env.funding_rate   = (funding_ && funding_->ready()) ? funding_->rate() : 0.0;
        cap_env.latency_ms     = market_env_.latency_ms;
        cap_env.net_clean      = market_env_.net_clean;
        
        double unrealized_bp = 0.0;
        double drawdown_bp = 0.0;
        
        // Derive engine label for per-engine win-rate boost
        const char* ccl_engine =
            (layer == LAYER_LEADLAG)         ? "LEADLAG"    :
            (layer == LAYER_LEADLAG_ETH_SOL) ? "LL-ETH-SOL" :
            (layer == LAYER_IMPULSE)         ? "IMPULSE"    :
            (layer == LAYER_EXPANSION)       ? "EXPAND"     :
            (layer == LAYER_LIQUIDATION)     ? "LIQ"        :
            (layer == LAYER_FUNDING)         ? "FUND"       :
            (layer == LAYER_NGAS)            ? "NGAS"       :
            (layer == LAYER_ETH_LEAD)        ? "ETH-LEAD"   :
            (layer == LAYER_SOL_LEAD)        ? "SOL-LEAD"   :
            (layer == LAYER_VOLSHOCK)        ? "VOLSHOCK"   :
            (layer == LAYER_OFI)             ? "OFI"        :
            (layer == LAYER_SWEEP)           ? "SWEEP"      :
            (layer == LAYER_MM_PRESSURE)     ? "MM-PRESSURE": "UNKNOWN";
        double final_size = capital_control_.compute_final_size(
            final_weight, cap_env, unrealized_bp, drawdown_bp, ccl_engine
        );
        
        double vol_score = vol_scoring_[id].get_vol_score();
        double legacy_size_mult = vol_scoring_[id].get_size_multiplier(
            vol_score, 
            latency_gov_.regime() == NET_CLEAN ? 5.0 : 15.0
        );
        
        //  POSITION SIZING MULTIPLIERS 
        // Single consolidated block  net multiplier per engine/symbol combo.
        // Based on session data: LEADLAG 80%+ WR +3.8bp avg, IMPULSE 70% WR,
        // EXPAND marginal, ETH weakest performer across all engines.

        // Per-engine multiplier
        // LEADLAG: near-100% WR in history  run at max leverage (4x)
        // eng_mult: position size multiplier per layer
        // All aggressive multipliers removed until net-positive edge confirmed
        // over 50+ trades. LEADLAG was 4x on gross-positive/net-negative edge.
        double eng_mult = (layer == LAYER_LIQUIDATION)      ? 2.0 :  // event-driven, capped until live confirmed
                          (layer == LAYER_FUNDING)          ? 1.5 :  // slow-burn
                          (layer == LAYER_NGAS)             ? 1.0 :  // macro, unproven net
                          (layer == LAYER_LEADLAG)          ? 1.0 :  // BUG5 FIX: was 4x, net-negative at 47% WR
                          (layer == LAYER_LEADLAG_ETH_SOL)  ? 0.5 :  // net-negative until proven (see audit)
                          (layer == LAYER_IMPULSE)          ? 1.0 :  // neutral until net confirmed
                          (layer == LAYER_EXPANSION)        ? 0.8 :  // BUG10: net-negative, reduce exposure
                          (layer == LAYER_ETH_LEAD)         ? 1.0 :  // unproven, neutral
                          (layer == LAYER_SOL_LEAD)         ? 1.0 :  // unproven, neutral
                          (layer == LAYER_VOLSHOCK)         ? 0.5 :  // new engine, conservative
                          (layer == LAYER_OFI)              ? 0.5 :  // new engine, conservative
                          (layer == LAYER_SWEEP)            ? 0.5 :  // new engine, taker — conservative
                          (layer == LAYER_MM_PRESSURE)      ? 0.5 :  // new engine, conservative
                          (layer == LAYER_VWAP)             ? 1.0 :
                                                              1.0;
        legacy_size_mult *= eng_mult;

        // Per-layer adaptive multiplier — data-driven, kicks in after MIN_TRADES.
        // Derived from LayerPerformanceTracker EMA (EdgeReinforcement pattern).
        // Bounded [0.40, 1.50] so it can never zero-out a layer or over-lever.
        {
            double layer_mult = layer_tracker_.multiplier(layer);
            if (layer_mult != 1.0) {
                std::printf("[LAYER-ADAPT] %s layer=%d trades=%d pnl_ema=%.2fbp mult=%.2f\n",
                    sym_short(id), (int)layer,
                    layer_tracker_.trade_count(layer),
                    layer_tracker_.pnl_ema(layer),
                    layer_mult);
                std::fflush(stdout);
                legacy_size_mult *= layer_mult;
            }
        }

        // Per-symbol multiplier  data driven
        // SOL: best performer. ETH: weakest. New symbols (BNB/AVAX/LINK/POL): neutral until data.
        // sym_mult: per-symbol size bias
        // BUG6 FIX: AVAX/LINK/POL was 4x — thin books + few trades = unacceptable live risk
        // Capped at 1.5 until 50+ live net-positive trades confirmed per symbol
        double sym_mult = (id == 4 || id == 5 || id == 6) ? 1.5 :  // AVAX/LINK/POL: thin books, capped until live confirmed
                          (id == 2) ? 1.2 :   // SOL: good WR, modest boost
                          (id == 1) ? 0.7 :   // ETH: weakest performer, suppressed
                                      1.0;    // BTC/BNB: neutral
        legacy_size_mult *= sym_mult;

        if (consecutive_losses_ >= 2) {
            legacy_size_mult *= 0.6;
        }

        // FUNDING RATE ADJUSTMENT  reduce long size when longs are crowded
        // High positive funding = overcrowded longs = lower expected edge
        if (funding_ && funding_->ready()) {
            double fund_mult = funding_->long_size_multiplier();
            if (fund_mult != 1.0) {
                std::printf("[FUNDING-ADJ] size_mult %.2f -> %.2f (funding=%.5f%%)\n", legacy_size_mult, legacy_size_mult * fund_mult, funding_->rate() * 100.0);
                std::fflush(stdout);
            }
            legacy_size_mult *= fund_mult;
        }
        
        const char* sym  = sym_short(id);
        const char* mode = (layer == LAYER_MICRO)    ? "IMBAL"   :
                           (layer == LAYER_IMPULSE)  ? "IMPULSE" :
                           (layer == LAYER_LEADLAG)  ? "LEADLAG" :
                           (layer == LAYER_VACUUM)          ? "VACUUM"    :
                           (layer == LAYER_VWAP)              ? "VWAP"      :
                           (layer == LAYER_LEADLAG_ETH_SOL)   ? "LL-ETH-SOL" :
                           (layer == LAYER_LIQUIDATION)      ? "LIQ"        :
                           (layer == LAYER_FUNDING)          ? "FUND"       :
                           (layer == LAYER_NGAS)             ? "NGAS"       :
                           (layer == LAYER_ETH_LEAD)         ? "ETH-LEAD"   :
                           (layer == LAYER_SOL_LEAD)         ? "SOL-LEAD"   :
                           (layer == LAYER_VOLSHOCK)         ? "VOLSHOCK"   :
                           (layer == LAYER_OFI)             ? "OFI"        :
                           (layer == LAYER_SWEEP)           ? "SWEEP"      :
                           (layer == LAYER_MM_PRESSURE)     ? "MM-PRESSURE": "EXPAND";

        const bool shadow_mode = (executor_ && executor_->is_shadow());
        const bool use_maker = TradingConfig::MAKER_MODE && !shadow_mode;
        if (use_maker) {
            //  MAKER MODE: post limit order 
            // Do NOT open position yet. Set state to PENDING.
            // manage_pending() will open position when/if limit is filled.
            double bid = s.last_tick.bid > 0.0 ? s.last_tick.bid : price;
            double ask = s.last_tick.ask > 0.0 ? s.last_tick.ask : price * 1.0001;

            // Map LayerMode to int id expected by LimitOrderManager
            // 0=IMBALANCE(maker/bid), 1=IMPULSE(taker/ask), 2=EXPANSION(maker/mid), 3=LEADLAG(taker/ask)
            // 4=LEADLAG-MAKER: post at ask-0.1*spread (aggressive maker, ~4bp saving vs taker)
            int layer_int = (layer == LAYER_MICRO)              ? 0 :  // maker: post at bid
                            (layer == LAYER_IMPULSE)             ? 1 :  // taker: post at ask (DISABLED)
                            (layer == LAYER_EXPANSION)           ? 2 :  // maker: post at mid-0.3*spread (DISABLED)
                            (layer == LAYER_LEADLAG)             ? 4 :  // MAKER: post at ask-0.1*spread (saves 4bp)
                            (layer == LAYER_LEADLAG_ETH_SOL)     ? 4 :  // MAKER: same aggressive maker
                            (layer == LAYER_ETH_LEAD)            ? 3 :  // taker (DISABLED)
                            (layer == LAYER_SOL_LEAD)            ? 3 :  // taker (DISABLED)
                            (layer == LAYER_VOLSHOCK)            ? 3 :  // taker
                                                                   3;   // default taker

            limit_orders_[id].enter_pending(layer_int, bid, ask, ts);

            s.pos.state         = POS_PENDING;
            s.pos.pending_layer = layer;
            s.pos.is_long       = is_long;
            open_positions_++;  // Reserve  decremented if cancelled

            if (layer == LAYER_EXPANSION) {
                expand_state_[id] = 1;
                expand_entry_price_[id] = price;
                expand_peak_price_[id]  = price;
            }

            std::printf("[ENTER-PENDING] %s | %s | regime=%s | signal_px=%.4f | bid=%.4f | weight=%.3f\n",
                sym, mode, regime_name(s.regime), price, bid, final_weight);
            std::fflush(stdout);

        } else {
            //  TAKER MODE: open position immediately 
            s.pos.state       = POS_OPEN;
            s.pos.entry_price = price;
            s.pos.entry_ts    = ts;
            s.pos.layer       = layer;
            s.pos.is_long     = is_long;
            s.pos.open_ticks  = 0;
            s.pos.peak_price  = price;
            s.pos.mfe         = 0.0;
            s.pos.mae         = 0.0;

            // Capture entry context for shadow log
            s.entry_imbalance  = s.last_tick.book_imbalance;
            s.entry_flow_ratio = compute_flow_ratio(id);
            s.entry_spread_bps = s.last_tick.spread_bps;
            s.entry_btc_move   = leadlag_.btc_move_bp();
            s.entry_latency_ms = market_env_.latency_ms;

            if (layer == LAYER_EXPANSION) {
                expand_state_[id] = 1;
                expand_entry_price_[id] = price;
                expand_peak_price_[id]  = price;
            }

            open_positions_++;

            std::printf("[ENTER] %s | %s | %s | regime=%s | px=%.4f | weight=%.3f | mult=%.2f\n",
                sym, mode, is_long ? "LONG" : "SHORT", regime_name(s.regime), price, final_weight, legacy_size_mult);
            std::fflush(stdout);

            // Execute order (shadow or live  determined by executor config)
            if (executor_) {
                double qty = (final_size * legacy_size_mult) / std::max(price, 1.0);
                executor_->execute(sym_lower(id), is_long, qty, price);
            }

            std::string symbol_full = sym_full(id);
            broadcast_to_gui(GuiMessageBuilder::position_enter(
                symbol_full, mode, price, (int)s.regime, final_weight
            ));
        }
    }
    
    void exit(int id, double pnl, int64_t ts, SymbolState& s) {
        const char* sym = sym_short(id);
        std::string symbol_full = sym_full(id);

        // NET PnL: subtract round-trip cost based on actual execution model
        // Taker layers (post at ask): 8bp round trip (4bp/side at VIP0)
        // Maker layers (post below mid): 4bp round trip (rebate ~1bp/side, spread ~2bp)
        // LEADLAG and LL-ETH-SOL now use aggressive maker entry (layer_id=4)
        // Cost ~4bp round trip (maker rebate), not 8bp taker
        bool is_taker_exit = (s.pos.layer == LAYER_IMPULSE         ||  // disabled but kept for correctness
                              s.pos.layer == LAYER_ETH_LEAD        ||  // disabled
                              s.pos.layer == LAYER_SOL_LEAD        ||  // disabled
                              s.pos.layer == LAYER_VOLSHOCK        ||
                              s.pos.layer == LAYER_LIQUIDATION);
        // LEADLAG/LL-ETH-SOL: aggressive maker — same cost tier as other maker layers
        bool is_leadlag_maker = (s.pos.layer == LAYER_LEADLAG ||
                                 s.pos.layer == LAYER_LEADLAG_ETH_SOL);
        double round_trip_cost = is_taker_exit    ? TradingConfig::TAKER_ROUND_TRIP_BP   // 8bp
                               : is_leadlag_maker ? TradingConfig::MAKER_ROUND_TRIP_BP   // 4bp (aggressive maker)
                               :                    TradingConfig::MAKER_ROUND_TRIP_BP;  // 4bp
        double pnl_net = pnl - round_trip_cost;

        int64_t hold_time_ms = ts - s.pos.entry_ts;  // ts is already milliseconds
        double current_latency = snapshots_[id].lat_p95_ms;
        double slippage_bps = 2.0;

        pnl_by_band_.record_trade(symbol_full, current_latency, pnl_net, slippage_bps);

        realized_pnl_ += pnl_net;
        total_pnl_ = realized_pnl_;
        total_trades_++;
        
        // From here on, use pnl_net for all recording/display
        // pnl (raw gross move) kept only for price reconstruction in shadow log
        const char* layer_label =
                            (s.pos.layer == LAYER_MICRO)          ? "IMBAL"      :
                            (s.pos.layer == LAYER_IMPULSE)        ? "IMPULSE"    :
                            (s.pos.layer == LAYER_EXPANSION)      ? "EXPAND"     :
                            (s.pos.layer == LAYER_LEADLAG)        ? "LEADLAG"    :
                            (s.pos.layer == LAYER_LEADLAG_ETH_SOL)? "LL-ETH-SOL" :
                            (s.pos.layer == LAYER_VACUUM)         ? "VACUUM"     :
                            (s.pos.layer == LAYER_VWAP)           ? "VWAP"       :
                            (s.pos.layer == LAYER_LIQUIDATION)    ? "LIQ"        :
                            (s.pos.layer == LAYER_FUNDING)        ? "FUND"       :
                            (s.pos.layer == LAYER_NGAS)           ? "NGAS"       :
                            (s.pos.layer == LAYER_ETH_LEAD)       ? "ETH-LEAD"   :
                            (s.pos.layer == LAYER_SOL_LEAD)       ? "SOL-LEAD"   :
                            (s.pos.layer == LAYER_VOLSHOCK)       ? "VOLSHOCK"   :
                            (s.pos.layer == LAYER_OFI)             ? "OFI"        :
                            (s.pos.layer == LAYER_SWEEP)           ? "SWEEP"      :
                            (s.pos.layer == LAYER_MM_PRESSURE)     ? "MM-PRESSURE": "UNKNOWN";
        const char* win_str = pnl_net > 0 ? "WIN" : "LOSS";

        std::printf("[EXIT] %s | %s | %s | reason=%s | pnl=%.2fbp | mfe=%.2f | mae=%.2f | lat=%.1fms | hold=%ldms | total_pnl=%.2f\n",
            sym, layer_label, win_str,
            pending_exit_reason_.empty() ? "?" : pending_exit_reason_.c_str(),
            pnl_net, s.pos.mfe, s.pos.mae, current_latency, hold_time_ms, total_pnl_);
        std::fflush(stdout);

        // Compute exit reason before the callback block so it's in scope for
        // both stats_for() and the per-symbol circuit breaker below
        std::string exit_reason = pending_exit_reason_.empty() ? (pnl >= 0 ? "TP" : "SL") : pending_exit_reason_;

        // Fire trade exit callback so QuadEngine can log full trade data
        if (trade_exit_cb_) {
            TradeExitData td;
            td.symbol      = sym;
            td.engine      = layer_label;
            td.pnl_bp      = pnl_net;
            td.entry_price = s.pos.entry_price;
            td.exit_price  = s.pos.entry_price * (1.0 + pnl / 10000.0);
            td.mfe_bp      = s.pos.mfe;
            td.mae_bp      = s.pos.mae;
            td.hold_ms     = hold_time_ms;
            td.reason      = exit_reason;
            trade_exit_cb_(td);
        }
        pending_exit_reason_.clear();

        // Record per-layer session stats (visible in GUI Session Stats panel)
        stats_for(s.pos.layer).record(pnl_net, exit_reason, s.pos.mfe, s.pos.mae);
        pnl_governor_.record(pnl_net);  // BUG2 FIX: daily loss governor (resets at UTC midnight)
        if (pnl_governor_.blocked()) {
            std::printf("[DAILY-KILL] Daily loss limit reached (%.1fbp) — halting new entries until UTC midnight\n", -120.0);
            std::fflush(stdout);
            kill_until_ = ts + 24 * 3600 * 1000LL;  // effectively stop until next restart / midnight reset
        }
        capital_control_.record_trade_result(std::string(layer_label), pnl_net > 0.0);
        capital_control_.update_compounding_equity(10000.0 + realized_pnl_);  // per-engine win-rate boost

        // Shadow log  structured CSV for edge measurement
        {
            ShadowEntry se;
            se.ts_enter      = s.pos.entry_ts;
            se.ts_exit       = ts;
            std::strncpy(se.symbol, sym, sizeof(se.symbol)-1);
            std::strncpy(se.layer,  layer_label, sizeof(se.layer)-1);
            std::strncpy(se.regime,
                (s.regime == REGIME_GRIND)    ? "GRIND"    :
                (s.regime == REGIME_BUILDUP)  ? "BUILDUP"  :
                (s.regime == REGIME_BREAKOUT) ? "BREAKOUT" : "DEAD",
                sizeof(se.regime)-1);
            se.entry_px      = s.pos.entry_price;
            se.exit_px       = s.pos.entry_price * (1.0 + pnl / 10000.0);
            se.pnl_bp        = pnl_net;   // net after round-trip cost
            se.gross_bp      = pnl;        // raw price move before cost
            se.mfe_bp        = s.pos.mfe;
            se.mae_bp        = s.pos.mae;
            se.hold_ms       = hold_time_ms;
            se.latency_ms    = current_latency;
            se.imbalance     = s.entry_imbalance;
            se.flow_ratio    = s.entry_flow_ratio;
            se.spread_bps    = s.entry_spread_bps;
            se.btc_move_bp   = s.entry_btc_move;
            se.win           = (pnl > 0) ? 1 : 0;
            shadow_log_.record(se);
        }

        // Execute closing order (shadow or live)
        if (executor_) {
            double exit_px = s.pos.entry_price * (1.0 + pnl / 10000.0);
            double qty = (s.pos.entry_price > 0.0)
                ? (capital_control_.compute_final_size(0.5,
                      CapitalControlLayer::MarketEnv{}, 0.0, 0.0, "UNKNOWN") / s.pos.entry_price)
                : 0.0;
            // Spot long-only: always SELL to close
            executor_->execute(sym, false /*sell*/, qty, exit_px);
        }
        
        // Broadcast exit to GUI with complete trade details
        const char* layer_str = (s.pos.layer == LAYER_MICRO)   ? "MICRO"   :
                               (s.pos.layer == LAYER_IMPULSE)  ? "IMPULSE" :
                               (s.pos.layer == LAYER_LEADLAG)  ? "LEADLAG" :
                               (s.pos.layer == LAYER_VACUUM)        ? "VACUUM"    :
                               (s.pos.layer == LAYER_VWAP)          ? "VWAP"      :
                               (s.pos.layer == LAYER_LEADLAG_ETH_SOL)? "LL-ETH-SOL" :
                               (s.pos.layer == LAYER_LEADLAG)       ? "LEADLAG"   :
                               (s.pos.layer == LAYER_EXPANSION)     ? "EXPAND"    :
                               (s.pos.layer == LAYER_IMPULSE)       ? "IMPULSE"   :
                               (s.pos.layer == LAYER_LIQUIDATION)   ? "LIQ"       :
                               (s.pos.layer == LAYER_FUNDING)       ? "FUND"      :
                               (s.pos.layer == LAYER_NGAS)          ? "NGAS"      : "UNKNOWN";
        broadcast_to_gui(GuiMessageBuilder::position_exit(
            symbol_full, layer_str, pnl, hold_time_ms, current_latency
        ));
        
        // Also send trade info in format GUI expects
        std::ostringstream trade_msg;
        trade_msg << std::fixed << std::setprecision(2);
        trade_msg << "{\"type\":\"trade\","
                  << "\"last_order_symbol\":\"" << symbol_full << "\","
                  << "\"last_order_side\":\"" << (pnl > 0 ? "WIN" : "LOSS") << "\","
                  << "\"last_order_size\":0,"
                  << "\"last_order_price\":" << s.pos.entry_price << ","
                  << "\"last_order_conviction\":" << (pnl + 2.0) << ","  // pnl + spread
                  << "\"last_order_cost_floor\":2.0,"
                  << "\"last_order_time\":\"" << "now" << "\","
                  << "\"day_pnl\":" << total_pnl_
                  << "}";
        broadcast_to_gui(trade_msg.str());
        
        // PHASE 2: Update reinforcement layer
        AdaptiveReinforcementLayer::TradeResult tr;
        tr.regime = (int)regime_classifiers_[id].regime();
        tr.pnl_bps = pnl_net;
        tr.mfe_bps = s.pos.mfe;
        tr.mae_bps = s.pos.mae;
        reinforcement_.record_trade(tr);

        // Per-layer performance EMA — feeds sizing multiplier in enter()
        layer_tracker_.record(s.pos.layer, pnl_net);
        
        // Update allocator metrics
        if (s.pos.layer == LAYER_IMPULSE || s.pos.layer == LAYER_EXPANSION) {
            adaptive_allocator_.update_impulse_metrics(pnl_net, s.pos.mfe, s.pos.mae, 0.0);
        } else {
            adaptive_allocator_.update_maker_metrics(pnl_net, s.pos.mfe, s.pos.mae, 0.0);
        }
        
        LayerType layer_type = (s.pos.layer == LAYER_IMPULSE) ? LayerType::IMPULSE :
                               (s.pos.layer == LAYER_EXPANSION) ? LayerType::EXPAND :
                               (s.pos.layer == LAYER_MICRO) ? LayerType::MICRO : LayerType::LEADLAG;
        governor_.record_trade_result(symbol_full, layer_type, pnl_net);
        
        if (pnl_net < 0) {
            loss_streak_++;
            consecutive_losses_++;
            last_loss_ts_ = ts;
            snapshots_[id].loss_streak++;
            snapshots_[id].last_disable_time = std::chrono::steady_clock::now();

            // PER-SYMBOL CIRCUIT BREAKER  track SL streak per symbol
            if (exit_reason == "SL") {
                sym_consecutive_sl_[id]++;
                if (sym_consecutive_sl_[id] >= SYM_SL_STREAK_LIMIT) {
                    const bool shadow_mode = (executor_ && executor_->is_shadow());
                    const int64_t pause_ms = shadow_mode ? 45000LL : SYM_SL_PAUSE_MS;
                    sym_sl_cooldown_[id] = ts + pause_ms;
                    const char* sym = sym_short(id);
                    std::printf("[CIRCUIT-BREAK-TRIGGER] %s | %d consecutive SLs  pausing %.0fs\n",
                        sym, sym_consecutive_sl_[id], pause_ms / 1000.0);
                    std::fflush(stdout);
                }
            } else {
                // TIMEOUT/other non-SL loss does NOT reset SL streak
                // Previously this reset to 0, allowing SLTIMEOUTSLTIMEOUT to bypass the
                // circuit breaker indefinitely. Now only a WIN resets the streak.
                // (sym_consecutive_sl_ stays at current count)
            }
        } else { // pnl_net >= 0
            loss_streak_ = 0;
            consecutive_losses_ = 0;
            snapshots_[id].loss_streak = 0;
            sym_consecutive_sl_[id] = 0;  // win resets the SL streak for this symbol
        }
        
        if (loss_streak_ >= 3) kill_until_ = ts + 5000;
        
        // Scale cooldown with consecutive losses  back off faster after streak
        // 0 losses: 500ms | 1 loss: 1000ms | 2: 2000ms | 3+: 5000ms
        int64_t cooldown_ms = 500;
        if (consecutive_losses_ >= 3) cooldown_ms = 5000;
        else if (consecutive_losses_ >= 2) cooldown_ms = 2000;
        else if (consecutive_losses_ >= 1) cooldown_ms = 1000;
        s.cooldown_until = ts + cooldown_ms;
        
        if (s.pos.layer == LAYER_EXPANSION) {
            expand_state_[id] = 0;
            expand_entry_price_[id] = 0.0;
            expand_peak_price_[id] = 0.0;
        }
        
        s.pos.reset();
        open_positions_--;
        
        if (open_positions_ == 0 && ts > layer_lock_until_) {
            system_state_ = SYS_IDLE;
        }
    }
    
    SymbolState symbols_[MAX_SYMBOLS];
    LatencyGovernor latency_gov_;
    LeadLagEngine      leadlag_;
    LiquidationEngine  liq_engine_;
    VolumeShockEngine  vol_shock_;
    LimitOrderManager limit_orders_[MAX_SYMBOLS];  // One per symbol
    ShadowLogger shadow_log_;
    FundingRateFetcher*  funding_ = nullptr;  // optional  set from main()
    NGASLeadLagEngine*   ngas_    = nullptr;  // optional  set from main()
    VolatilityScoring vol_scoring_[MAX_SYMBOLS];
    StatefulGovernor governor_;
    PnLGovernor      pnl_governor_;   // BUG2 FIX: daily loss limit with UTC midnight reset
    MultiSymbolAllocator allocator_;
    RejectionTelemetryAsync rejection_telemetry_;
    
    PnLByLatencyBand pnl_by_band_;
    EngineStallDetector stall_detector_;
    RejectionThrottle rejection_throttle_;
    
    SymbolSnapshot snapshots_[MAX_SYMBOLS];
    int64_t last_snapshot_update_[MAX_SYMBOLS];
    int64_t tick_count_[MAX_SYMBOLS];
    int64_t last_tick_count_reset_[MAX_SYMBOLS];
    int    open_positions_;
    double ll_offpeak_size_mult_ = 1.0;
    int loss_streak_;
    int64_t kill_until_;
    SystemState system_state_;
    int64_t layer_lock_until_;
    
    int expand_state_[MAX_SYMBOLS];
    double expand_entry_price_[MAX_SYMBOLS];
    double expand_peak_price_[MAX_SYMBOLS];
    int expand_confirm_ticks_[MAX_SYMBOLS];  // consecutive ticks above vol_ratio threshold
    int expand_post_compress_ticks_[MAX_SYMBOLS]; // ticks since COMPRESSIONBREAKOUT transition
    int consecutive_losses_;
    int64_t last_loss_ts_;

    // PER-SYMBOL CIRCUIT BREAKER  prevents entering a trending-against-us move
    // After 2 consecutive SL exits on the same symbol, pause that symbol 5 minutes
    // Prevents 02:46-02:51 style ETH crash cluster (8 x -8bp = -64bp in 5 min)
    int     sym_consecutive_sl_[MAX_SYMBOLS];
    int64_t sym_sl_cooldown_[MAX_SYMBOLS];
    int64_t shadow_probe_last_ms_[MAX_SYMBOLS] = {};
    static constexpr int     SYM_SL_STREAK_LIMIT = 2;
    static constexpr int64_t SYM_SL_PAUSE_MS     = 5 * 60000LL;

    double total_pnl_;
    double realized_pnl_;
    int total_trades_;

    //  PER-LAYER SESSION STATS 
    // Automatically updated in exit()  queried by GUI via get_session_stats_json()
    struct LayerStats {
        int wins          = 0;
        int losses        = 0;
        int tp_exits      = 0;
        int sl_exits      = 0;
        int trail_exits   = 0;
        int timeout_exits = 0;
        double total_pnl_bp = 0.0;
        double best_trade   = 0.0;
        double worst_trade  = 0.0;
        double sum_mfe      = 0.0;
        double sum_mae      = 0.0;

        void record(double pnl, const std::string& reason, double mfe, double mae) {
            if (pnl > 0) wins++; else losses++;
            total_pnl_bp += pnl;
            int n = wins + losses;
            if (n == 1 || pnl > best_trade)  best_trade  = pnl;
            if (n == 1 || pnl < worst_trade) worst_trade = pnl;
            sum_mfe += mfe;
            sum_mae += mae;
            if      (reason == "TP")      tp_exits++;
            else if (reason == "SL")      sl_exits++;
            else if (reason == "TRAIL")   trail_exits++;
            else if (reason == "TIMEOUT") timeout_exits++;
        }
        int    total()    const { return wins + losses; }
        double win_rate() const { return total() > 0 ? (double)wins / total() * 100.0 : 0.0; }
        double avg_pnl()  const { return total() > 0 ? total_pnl_bp / total() : 0.0; }
        double avg_mfe()  const { return total() > 0 ? sum_mfe / total() : 0.0; }
        double avg_mae()  const { return total() > 0 ? sum_mae / total() : 0.0; }
    };
    LayerStats layer_stats_[17]; // indexed by LayerMode enum value (0..16)
                                  // 14=LAYER_OFI  15=LAYER_SWEEP  16=LAYER_MM_PRESSURE

    LayerStats& stats_for(LayerMode m) {
        int idx = (int)m;
        return layer_stats_[(idx >= 0 && idx < 17) ? idx : 0];
    }
    
    GuiBroadcastCallback gui_broadcast_;
    TradeExitCallback    trade_exit_cb_;
    std::string          pending_exit_reason_;  // Set before each exit() call
    
    // PHASE 2: Microstructure and capital allocation
    BookState book_states_[MAX_SYMBOLS];
    MarketEnv market_env_;
    ToxicFlowDetector toxic_flow_[MAX_SYMBOLS];
    MicroEdgeEngine micro_edge_[MAX_SYMBOLS];
    HybridRegimeClassifier regime_classifiers_[MAX_SYMBOLS];
    AdaptiveAllocator adaptive_allocator_;
    CapitalControlLayer capital_control_;
    ExecutionOptimizer execution_optimizer_;
    AdaptiveReinforcementLayer reinforcement_;
    LayerPerformanceTracker    layer_tracker_;  // per-layer EMA sizing feedback

    // Depth baseline per symbol  used for real queue_density in cap_env
    double depth_baseline_[MAX_SYMBOLS];

    // Spot executor  wired at startup via set_executor()
    SpotExecutor* executor_ = nullptr;

    static const char* sym_lower(int id) { return sym_full(id); }
};

}

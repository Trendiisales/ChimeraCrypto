#pragma once
#include "live/BinanceWSFeed.hpp"
#include "config/TradingConfig.hpp"
#include "LatencyGovernor.hpp"
#include "RegimeTypes.hpp"
#include "LeadLagEngine.hpp"
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

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <string>
#include <functional>
#include <deque>
#include <sstream>
#include <iomanip>

namespace chimera {

enum LayerMode {
    LAYER_NONE,
    LAYER_MICRO,
    LAYER_IMPULSE,
    LAYER_EXPANSION,
    LAYER_LEADLAG
};

enum PosState {
    POS_FLAT,
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
    int open_ticks;      // Ticks since position opened (for minimum hold time)
    double peak_price;   // Highest favorable price since entry (for trailing)
    
    // Phase 2: MFE/MAE tracking
    double mfe;
    double mae;

    void reset() {
        state = POS_FLAT;
        entry_price = 0.0;
        entry_ts = 0;
        layer = LAYER_NONE;
        open_ticks = 0;
        peak_price = 0.0;
        mfe = 0.0;
        mae = 0.0;
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

class BalancedEngine {
public:
    using GuiBroadcastCallback = std::function<void(const std::string&)>;
    
    BalancedEngine() : governor_(GovernorConfig()), allocator_(AllocatorConfig()), gui_broadcast_(nullptr) {
        for (int i = 0; i < 3; ++i)
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
        for (int i = 0; i < 3; ++i) {
            last_snapshot_update_[i] = 0;
            tick_count_[i] = 0;
            last_tick_count_reset_[i] = 0;
            snapshots_[i].symbol = (i == 0) ? "btcusdt" : (i == 1) ? "ethusdt" : "solusdt";
            snapshots_[i].last_disable_time = std::chrono::steady_clock::now();
        }
        
        for (int i = 0; i < 3; ++i) {
            expand_state_[i] = 0;
            expand_entry_price_[i] = 0.0;
            expand_peak_price_[i] = 0.0;
        }
        
        // Phase 2: Initialize capital control
        capital_control_.set_base_capital(10000.0);
        
        std::printf("╔════════════════════════════════════════════════════════════════╗\n");
        std::printf("║      BALANCED ENGINE PHASE 2 - INSTITUTIONAL REBUILD          ║\n");
        std::printf("╠════════════════════════════════════════════════════════════════╣\n");
        std::printf("║ Microstructure Analysis: ENABLED                              ║\n");
        std::printf("║ Toxic Flow Detection: ENABLED                                 ║\n");
        std::printf("║ Hybrid Regime Classifier: ENABLED                             ║\n");
        std::printf("║ Adaptive Allocator: ENABLED (5ms loop)                        ║\n");
        std::printf("║ Capital Control Layer: ENABLED                                ║\n");
        std::printf("║ Execution Optimizer: ENABLED                                  ║\n");
        std::printf("║ Reinforcement Layer: ENABLED                                  ║\n");
        std::printf("║ MICRO Layer: PARKED (measurement only)                        ║\n");
        std::printf("╚════════════════════════════════════════════════════════════════╝\n");
        std::fflush(stdout);
    }

    inline void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        stall_detector_.on_ws_receive();
        stall_detector_.on_eval_start();
        
        std::string my_symbol = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        rejection_telemetry_.recordEvaluation(my_symbol);
        
        // PHASE 2: Update microstructure engines
        update_market_data(id, tick, ts, latency_ms);
        
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
                  << "\"governor\":\"ACTIVE\""
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
            
            for (int i = 0; i < 3; ++i) {
                const char* sym = (i == 0) ? "BTC" : (i == 1) ? "ETH" : "SOL";
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
                
                std::string sym_full = (i == 0) ? "btcusdt" : (i == 1) ? "ethusdt" : "solusdt";
                broadcast_to_gui(GuiMessageBuilder::regime_update(
                    sym_full, symbols_[i].regime, symbols_[i].last_price,
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
        Regime old_regime = s.regime;
        s.regime = classify_regime(id);
        
        if (s.regime != old_regime) {
            const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
            std::printf("[REGIME-CHANGE] %s: %s → %s\n", 
                       sym, regime_name(old_regime), regime_name(s.regime));
            std::fflush(stdout);
        }
        
        // Position management
        if (s.pos.state == POS_OPEN) {
            // Track MFE/MAE
            double move = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
            s.pos.mfe = std::max(s.pos.mfe, move);
            s.pos.mae = std::min(s.pos.mae, move);
            
            manage_position(id, price, ts, s);
            return;
        }
        
        // Signal evaluation
        if (ts < kill_until_) return;
        if (ts < s.cooldown_until) return;
        if (open_positions_ >= 1) return;
        
        // Try signals in priority order
        if (check_impulse(id, price, ts, s, latency_ms)) return;
        if (check_expansion(id, price, ts, s, latency_ms)) return;
        if (check_leadlag(id, price, ts, s, latency_ms)) return;
    }
    
    std::string get_rejection_stats() const { return rejection_telemetry_.build_json_snapshot(); }
    double get_total_pnl() const { return total_pnl_; }
    double get_realized_pnl() const { return realized_pnl_; }
    int get_total_trades() const { return total_trades_; }
    int get_open_positions() const { return open_positions_; }
    
    void set_gui_broadcast(GuiBroadcastCallback callback) {
        // DISABLED - GUI decoupled, logs only
        // gui_broadcast_ = callback;
    }
    
private:
    // PHASE 2: Market data update
    // tick carries REAL bid/ask/depth/trade data from the live feed.
    // No fake constants. If a field is 0.0, the feed hasn't sent it yet —
    // we use it as-is; the EMA-based engines self-initialise gracefully.
    void update_market_data(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
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
        market_env_.net_clean   = (latency_ms < 8.0);

        // ---- AdaptiveAllocator ----
        AdaptiveAllocator::Environment alloc_env;
        alloc_env.short_range = market_env_.short_range;
        alloc_env.long_range  = market_env_.long_range;
        alloc_env.spread_bps  = tick.spread_bps;
        alloc_env.latency_ms  = latency_ms;
        alloc_env.net_clean   = market_env_.net_clean;
        adaptive_allocator_.tick(alloc_env);
    }
    
    void report_phase2_metrics() {
        std::printf("\n[PHASE2-METRICS] Allocation & Microstructure:\n");
        std::printf("  Allocator: Impulse=%.3f | Maker=%.3f\n",
                   adaptive_allocator_.impulse_weight(),
                   adaptive_allocator_.maker_weight());
        
        for (int i = 0; i < 3; ++i) {
            const char* sym = (i == 0) ? "BTC" : (i == 1) ? "ETH" : "SOL";
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
            const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
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
            const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
            std::printf("[REGIME-CHANGE] %s: %s → %s (vol_ratio_smooth=%.3f after %d ticks)\n",
                       sym, regime_name(s.regime), regime_name(new_regime), vol_ratio, s.regime_ticks);
            std::fflush(stdout);
            s.regime_ticks = 0;
            
            // Set anchor price when entering BUILDUP or BREAKOUT for displacement confirmation
            if (new_regime == REGIME_BUILDUP || new_regime == REGIME_BREAKOUT) {
                s.regime_anchor_price = s.last_price;
            }
        }
        
        return new_regime;
    }
    
    bool check_impulse(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        std::string key = std::string((id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL") + " IMPULSE";
        
        if (latency_ms > TradingConfig::LATENCY_HARD_LIMIT_MS) {
            rejection_throttle_.record(key, "high_latency");
            return false;
        }
        
        if (s.regime == REGIME_BREAKOUT && s.short_returns.size() >= TradingConfig::IMPULSE_MIN_SHORT_TICKS) {
            // VOLATILITY FLOOR GATE - Prevent trading in cost-dominated regimes
            // Use EMA-based long_vol (adaptive baseline)
            double long_vol = s.long_vol_ema;
            if (long_vol < TradingConfig::MIN_LONG_VOL_FOR_TRADING) {
                rejection_throttle_.record(key, "low_long_vol");
                return false;
            }
            
            // DISPLACEMENT CONFIRMATION - Require minimum price movement from regime anchor
            double displacement = std::abs(price - s.regime_anchor_price);
            double min_required = TradingConfig::MIN_DISPLACEMENT_LONG_MULT * long_vol;
            
            if (displacement < min_required) {
                rejection_throttle_.record(key, "insufficient_displacement");
                return false;
            }
            
            enter(id, price, ts, s, LAYER_IMPULSE);
            return true;
        }
        
        rejection_throttle_.record(key, "no_breakout");
        return false;
    }
    
    bool check_expansion(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        std::string key = std::string((id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL") + " EXPAND";
        
        if (expand_state_[id] == 1) {
            rejection_throttle_.record(key, "already_in_expand");
            return false;
        }
        
        if (s.regime == REGIME_BUILDUP || s.regime == REGIME_BREAKOUT) {
            // Recompute volatility ratio using EMA-based long_vol
            double short_vol = compute_volatility(s.short_returns);
            double long_vol = s.long_vol_ema;  // Use EMA-based adaptive baseline
            
            // VOLATILITY FLOOR GATE - Prevent trading in cost-dominated regimes
            if (long_vol < TradingConfig::MIN_LONG_VOL_FOR_TRADING) {
                rejection_throttle_.record(key, "low_long_vol");
                return false;
            }
            
            double vol_ratio = (long_vol > TradingConfig::VOL_MIN_LONG) ? (short_vol / long_vol) : 0.0;
            
            if (vol_ratio > TradingConfig::EXPANSION_VOL_RATIO && s.short_returns.size() >= TradingConfig::EXPANSION_MIN_SHORT_TICKS) {
                // DISPLACEMENT CONFIRMATION - Require minimum price movement from regime anchor
                double displacement = std::abs(price - s.regime_anchor_price);
                double min_required = TradingConfig::MIN_DISPLACEMENT_LONG_MULT * long_vol;
                
                if (displacement < min_required) {
                    rejection_throttle_.record(key, "insufficient_displacement");
                    return false;
                }
                
                enter(id, price, ts, s, LAYER_EXPANSION);
                return true;
            }
        }
        
        rejection_throttle_.record(key, "weak_volatility");
        return false;
    }
    
    bool check_leadlag(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (ts < layer_lock_until_) return false;
        if (latency_ms > 12.0) return false;
        return false;
    }
    
    void manage_position(int id, double price, int64_t ts, SymbolState& s) {
        // Increment hold time
        s.pos.open_ticks++;
        
        // Calculate current P&L in bp
        double move_bp = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
        
        // Update MFE (Maximum Favorable Excursion) and MAE (Maximum Adverse Excursion)
        s.pos.mfe = std::max(s.pos.mfe, move_bp);
        s.pos.mae = std::min(s.pos.mae, move_bp);
        
        // CRITICAL: MFE SCRATCH - Exit if no profit after 8ms
        // This prevents SOL-style losses: mfe=0.00, mae=-9.26bp in 30ms
        int64_t hold_ms = (ts - s.pos.entry_ts) / 1000;
        if (hold_ms > 8 && s.pos.mfe < 0.01) {
            // No immediate followthrough - wrong-side entry, scratch it
            exit(id, move_bp, ts, s);
            return;
        }
        
        // Update peak price for trailing
        if (move_bp > 0) {  // In profit
            s.pos.peak_price = std::max(s.pos.peak_price, price);
        }
        
        // VOLATILITY-NORMALIZED TRAILING EXIT
        // Use EMA-based long_vol (adaptive baseline)
        double long_vol = s.long_vol_ema;
        double trail_distance = TradingConfig::TRAIL_LONG_VOL_MULT * long_vol;
        
        // Calculate peak profit in bp
        double peak_profit_bp = (s.pos.peak_price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
        
        // If we've achieved minimum profit, enable trailing stop
        if (peak_profit_bp >= TradingConfig::MIN_PROFIT_TO_TRAIL_BP) {
            // Check if price has retraced from peak by more than trail distance
            if (price < s.pos.peak_price - trail_distance) {
                exit(id, move_bp, ts, s);
                return;
            }
        }
        
        // SAFETY STOPS - Prevent catastrophic loss
        double max_loss_bp = -15.0;  // Hard stop at -15bp
        if (move_bp <= max_loss_bp) {
            exit(id, move_bp, ts, s);
            return;
        }
        
        // MINIMUM HOLD TIME CHECK - Prevent microstructure noise exits
        if (s.pos.open_ticks < TradingConfig::MIN_HOLD_TICKS) {
            return;  // Don't allow exit yet
        }
        
        // TIME-BASED EMERGENCY EXIT - Prevent stale positions
        int64_t max_hold_ms = 30000;  // 30 seconds max
        if (ts - s.pos.entry_ts > max_hold_ms) {
            exit(id, move_bp, ts, s);
            return;
        }
    }
    
    void enter(int id, double price, int64_t ts, SymbolState& s, LayerMode layer) {
        Signal sig;
        sig.symbol = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        sig.layer = (layer == LAYER_IMPULSE) ? LayerType::IMPULSE :
                    (layer == LAYER_EXPANSION) ? LayerType::EXPAND :
                    (layer == LAYER_MICRO) ? LayerType::MICRO : LayerType::LEADLAG;
        sig.expected_bps = (layer == LAYER_MICRO) ? 15.0 :
                          (layer == LAYER_IMPULSE) ? 18.0 :
                          (layer == LAYER_LEADLAG) ? 12.0 : 30.0;
        sig.confidence = 1.0;
        
        // CRITICAL: HARD COST FLOOR GATE
        // Enforces economic minimum - trade only if expected edge > total costs
        // Total cost = spread (2bp) + slippage (2bp) + commission (2.5bp) = 6.5bp
        static constexpr double COST_FLOOR_BP = 6.5;
        
        if (sig.expected_bps < COST_FLOOR_BP) {
            // HARD REJECT - insufficient expected edge to beat costs
            std::string key = std::string((id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL") + 
                             " " + ((layer == LAYER_IMPULSE) ? "IMPULSE" : 
                                    (layer == LAYER_EXPANSION) ? "EXPAND" : "OTHER");
            rejection_throttle_.record(key, "cost_floor");
            std::printf("[COST-FLOOR] Rejected %s | expected=%.2fbp < floor=%.2fbp\n",
                       key.c_str(), sig.expected_bps, COST_FLOOR_BP);
            std::fflush(stdout);
            return;  // HARD STOP - do not proceed
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
        
        // Compute final size via CapitalControlLayer
        CapitalControlLayer::MarketEnv cap_env;
        cap_env.short_range = market_env_.short_range;
        cap_env.long_range = market_env_.long_range;
        cap_env.spread_bps = 2.0;
        cap_env.book_imbalance = 0.0;
        cap_env.queue_density = 1.0;
        cap_env.funding_rate = 0.0;
        cap_env.latency_ms = market_env_.latency_ms;
        cap_env.net_clean = market_env_.net_clean;
        
        double unrealized_bp = 0.0;
        double drawdown_bp = 0.0;
        
        double final_size = capital_control_.compute_final_size(
            final_weight, cap_env, unrealized_bp, drawdown_bp
        );
        
        double vol_score = vol_scoring_[id].get_vol_score();
        double legacy_size_mult = vol_scoring_[id].get_size_multiplier(
            vol_score, 
            latency_gov_.regime() == NET_CLEAN ? 5.0 : 15.0
        );
        
        if (consecutive_losses_ >= 2) {
            legacy_size_mult *= 0.6;
        }
        
        s.pos.state = POS_OPEN;
        s.pos.entry_price = price;
        s.pos.entry_ts = ts;
        s.pos.layer = layer;
        s.pos.open_ticks = 0;
        s.pos.peak_price = price;  // Initialize peak for trailing
        s.pos.mfe = 0.0;
        s.pos.mae = 0.0;
        
        if (layer == LAYER_EXPANSION) {
            expand_state_[id] = 1;
            expand_entry_price_[id] = price;
            expand_peak_price_[id] = price;
        }
        
        open_positions_++;
        
        const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
        const char* mode = (layer == LAYER_MICRO) ? "MICRO" : 
                          (layer == LAYER_IMPULSE) ? "IMPULSE" : 
                          (layer == LAYER_LEADLAG) ? "LEADLAG" : "EXPAND";
        std::printf("[ENTER] %s | layer=%s | regime=%s | px=%.2f | weight=%.3f | legacy_mult=%.2f\n", 
            sym, mode, regime_name(s.regime), price, final_weight, legacy_size_mult);
        std::fflush(stdout);
        
        // Broadcast entry to GUI
        std::string symbol_full = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        broadcast_to_gui(GuiMessageBuilder::position_enter(
            symbol_full, mode, price, (int)s.regime, final_weight
        ));
    }
    
    void exit(int id, double pnl, int64_t ts, SymbolState& s) {
        const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
        std::string symbol_full = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        
        int64_t hold_time_ms = (ts - s.pos.entry_ts) / 1000;
        double current_latency = snapshots_[id].lat_p95_ms;
        double slippage_bps = 2.0;
        
        pnl_by_band_.record_trade(symbol_full, current_latency, pnl, slippage_bps);
        
        realized_pnl_ += pnl;
        total_pnl_ = realized_pnl_;
        total_trades_++;
        
        std::printf("[EXIT] %s | pnl=%.2fbp | mfe=%.2f | mae=%.2f | lat=%.1fms | hold=%ldms | total_pnl=%.2f\n", 
            sym, pnl, s.pos.mfe, s.pos.mae, current_latency, hold_time_ms, total_pnl_);
        std::fflush(stdout);
        
        // Broadcast exit to GUI with complete trade details
        const char* layer_str = (s.pos.layer == LAYER_MICRO) ? "MICRO" : 
                               (s.pos.layer == LAYER_IMPULSE) ? "IMPULSE" : 
                               (s.pos.layer == LAYER_LEADLAG) ? "LEADLAG" : "EXPAND";
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
        tr.pnl_bps = pnl;
        tr.mfe_bps = s.pos.mfe;
        tr.mae_bps = s.pos.mae;
        reinforcement_.record_trade(tr);
        
        // Update allocator metrics
        if (s.pos.layer == LAYER_IMPULSE || s.pos.layer == LAYER_EXPANSION) {
            adaptive_allocator_.update_impulse_metrics(pnl, s.pos.mfe, s.pos.mae, 0.0);
        } else {
            adaptive_allocator_.update_maker_metrics(pnl, s.pos.mfe, s.pos.mae, 0.0);
        }
        
        LayerType layer_type = (s.pos.layer == LAYER_IMPULSE) ? LayerType::IMPULSE :
                               (s.pos.layer == LAYER_EXPANSION) ? LayerType::EXPAND :
                               (s.pos.layer == LAYER_MICRO) ? LayerType::MICRO : LayerType::LEADLAG;
        governor_.record_trade_result(symbol_full, layer_type, pnl);
        
        if (pnl < 0) {
            loss_streak_++;
            consecutive_losses_++;
            last_loss_ts_ = ts;
            snapshots_[id].loss_streak++;
            snapshots_[id].last_disable_time = std::chrono::steady_clock::now();
        } else {
            loss_streak_ = 0;
            consecutive_losses_ = 0;
            snapshots_[id].loss_streak = 0;
        }
        
        if (loss_streak_ >= 3) kill_until_ = ts + 5000;
        
        s.cooldown_until = ts + 500;
        
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
    
    SymbolState symbols_[3];
    LatencyGovernor latency_gov_;
    LeadLagEngine leadlag_;
    VolatilityScoring vol_scoring_[3];
    StatefulGovernor governor_;
    MultiSymbolAllocator allocator_;
    RejectionTelemetryAsync rejection_telemetry_;
    
    PnLByLatencyBand pnl_by_band_;
    EngineStallDetector stall_detector_;
    RejectionThrottle rejection_throttle_;
    
    SymbolSnapshot snapshots_[3];
    int64_t last_snapshot_update_[3];
    int64_t tick_count_[3];
    int64_t last_tick_count_reset_[3];
    int open_positions_;
    int loss_streak_;
    int64_t kill_until_;
    SystemState system_state_;
    int64_t layer_lock_until_;
    
    int expand_state_[3];
    double expand_entry_price_[3];
    double expand_peak_price_[3];
    int consecutive_losses_;
    int64_t last_loss_ts_;
    
    double total_pnl_;
    double realized_pnl_;
    int total_trades_;
    
    GuiBroadcastCallback gui_broadcast_;
    
    // PHASE 2: Microstructure and capital allocation
    BookState book_states_[3];
    MarketEnv market_env_;
    ToxicFlowDetector toxic_flow_[3];
    MicroEdgeEngine micro_edge_[3];
    HybridRegimeClassifier regime_classifiers_[3];
    AdaptiveAllocator adaptive_allocator_;
    CapitalControlLayer capital_control_;
    ExecutionOptimizer execution_optimizer_;
    AdaptiveReinforcementLayer reinforcement_;
};

}

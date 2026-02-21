#pragma once
#include "LatencyGovernor.hpp"
#include "RegimeClassifier.hpp"
#include "LeadLagEngine.hpp"
#include "VolatilityScoring.hpp"
#include "Governor.hpp"
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
    
    // Phase 2: MFE/MAE tracking
    double mfe;
    double mae;

    void reset() {
        state = POS_FLAT;
        entry_price = 0.0;
        entry_ts = 0;
        layer = LAYER_NONE;
        mfe = 0.0;
        mae = 0.0;
    }
};

struct SymbolState {
    double last_price;
    double short_vol;
    double long_vol;
    int short_n;
    int long_n;

    Position pos;
    int64_t cooldown_until;
    Regime regime;

    void reset() {
        last_price = 0;
        short_vol = 0;
        long_vol = 0;
        short_n = 0;
        long_n = 0;
        cooldown_until = 0;
        regime = REGIME_DEAD;
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

    inline void on_tick(int id, double price, int64_t ts, double latency_ms) {
        stall_detector_.on_ws_receive();
        stall_detector_.on_eval_start();
        
        std::string my_symbol = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        rejection_telemetry_.recordEvaluation(my_symbol);
        
        // PHASE 2: Update microstructure engines
        update_market_data(id, price, ts, latency_ms);
        
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
                std::printf("[SYMBOL-STATE] %s | regime=%s (score=%.2f) | pos=%s | toxicity=%.2f | short_n=%d | long_n=%d\n",
                           sym, regime_name_str, regime_classifiers_[i].regime_score(),
                           (symbols_[i].pos.state == POS_OPEN ? "OPEN" : "FLAT"),
                           toxic_flow_[i].toxicity(),
                           symbols_[i].short_n, symbols_[i].long_n);
                
                std::string sym_full = (i == 0) ? "btcusdt" : (i == 1) ? "ethusdt" : "solusdt";
                broadcast_to_gui(GuiMessageBuilder::regime_update(
                    sym_full, symbols_[i].regime, symbols_[i].last_price,
                    symbols_[i].short_n, symbols_[i].short_vol, symbols_[i].long_vol
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
        
        // Update regime classifier
        auto& s = symbols_[id];
        s.last_price = price;
        s.short_n++;
        s.long_n++;
        
        if (s.short_n == 1) {
            s.short_vol = 0.0;
        } else {
            double d = price - symbols_[id].last_price;
            s.short_vol += d * d;
            if (s.short_n >= 10) {
                s.short_n = 0;
                s.short_vol = 0.0;
            }
        }
        
        if (s.long_n >= 200) {
            s.long_vol = s.short_vol;
            s.long_n = 0;
        }
        
        // Update regime based on RegimeClassifier
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
        gui_broadcast_ = callback;
    }
    
private:
    // PHASE 2: Market data update
    void update_market_data(int id, double price, int64_t ts, double latency_ms) {
        // Update ToxicFlowDetector
        ToxicFlowDetector::TickInput toxic_input;
        toxic_input.trade_volume = 1000.0;
        toxic_input.aggressive_buy_volume = 500.0;
        toxic_input.aggressive_sell_volume = 500.0;
        toxic_input.bid_depth = 50000.0;
        toxic_input.ask_depth = 50000.0;
        toxic_input.spread_bps = 2.0;
        toxic_input.short_range = symbols_[id].short_vol;
        toxic_input.long_range = symbols_[id].long_vol;
        toxic_flow_[id].update(toxic_input);
        
        // Update MicroEdgeEngine
        MicroEdgeEngine::BookState micro_state;
        micro_state.bid_depth = 50000.0;
        micro_state.ask_depth = 50000.0;
        micro_state.spread_bps = 2.0;
        micro_state.mid_price = price;
        micro_state.short_range = symbols_[id].short_vol;
        micro_state.long_range = symbols_[id].long_vol;
        micro_state.funding_rate = 0.0;
        micro_state.latency_ms = latency_ms;
        micro_edge_[id].update(micro_state);
        
        // Update HybridRegimeClassifier
        HybridRegimeClassifier::Input regime_input;
        regime_input.short_range = symbols_[id].short_vol;
        regime_input.long_range = symbols_[id].long_vol;
        regime_input.trade_volume = 1000.0;
        regime_input.aggressive_buy_volume = 500.0;
        regime_input.aggressive_sell_volume = 500.0;
        regime_input.bid_depth = 50000.0;
        regime_input.ask_depth = 50000.0;
        regime_input.spread_bps = 2.0;
        regime_input.latency_ms = latency_ms;
        regime_classifiers_[id].update(regime_input);
        
        // Update MarketEnv
        market_env_.short_range = (symbols_[0].short_vol + symbols_[1].short_vol + symbols_[2].short_vol) / 3.0;
        market_env_.long_range = (symbols_[0].long_vol + symbols_[1].long_vol + symbols_[2].long_vol) / 3.0;
        market_env_.vol_ratio = market_env_.short_range / std::max(market_env_.long_range, 1e-6);
        market_env_.latency_ms = latency_ms;
        market_env_.net_clean = (latency_ms < 8.0);
        
        // Update AdaptiveAllocator
        AdaptiveAllocator::Environment alloc_env;
        alloc_env.short_range = market_env_.short_range;
        alloc_env.long_range = market_env_.long_range;
        alloc_env.spread_bps = 2.0;
        alloc_env.latency_ms = latency_ms;
        alloc_env.net_clean = market_env_.net_clean;
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
        if (gui_broadcast_) {
            gui_broadcast_(message);
        }
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
    
    Regime classify_regime(int id) {
        auto& s = symbols_[id];
        if (s.long_n < 50) return REGIME_DEAD;
        
        double vol_ratio = s.short_vol / std::max(s.long_vol, 0.004);
        
        if (vol_ratio < 0.7) return REGIME_DEAD;
        if (vol_ratio < 1.3) return REGIME_GRIND;
        if (vol_ratio < 2.0) return REGIME_BUILDUP;
        return REGIME_BREAKOUT;
    }
    
    bool check_impulse(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        std::string key = std::string((id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL") + " IMPULSE";
        
        if (latency_ms > 8.0) {
            rejection_throttle_.record(key, "high_latency");
            return false;
        }
        
        if (s.regime == REGIME_BREAKOUT && s.short_n >= 5) {
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
            double vol_ratio = s.short_vol / std::max(s.long_vol, 0.004);
            if (vol_ratio > 1.5 && s.short_n >= 8) {
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
        double move = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
        
        // PHASE 2: Apply regime-based TP/SL adjustments
        double tp = 0, sl = 0;
        int64_t max_hold = 0;
        
        switch (s.pos.layer) {
            case LAYER_IMPULSE:
                tp = 18; sl = 8; max_hold = 5000; break;
            case LAYER_LEADLAG:
                tp = 12; sl = 6; max_hold = 1200; break;
            case LAYER_EXPANSION: {
                expand_peak_price_[id] = std::max(expand_peak_price_[id], price);
                double trail_distance = std::max(0.0012 * s.pos.entry_price, 
                                                 0.5 * (expand_peak_price_[id] - s.pos.entry_price));
                
                if (price <= expand_peak_price_[id] - trail_distance) {
                    exit(id, move, ts, s);
                    return;
                }
                
                tp = 30; sl = 15; max_hold = 10000; 
                break;
            }
            default:
                break;
        }
        
        // Apply regime adjustments
        tp *= regime_classifiers_[id].tp_adjustment();
        sl *= regime_classifiers_[id].stop_adjustment();
        
        // Apply toxic flow stop widening
        sl *= toxic_flow_[id].stop_widening_factor();
        
        if (move >= tp || move <= -sl || ts - s.pos.entry_ts > max_hold) {
            exit(id, move, ts, s);
        }
    }
    
    void enter(int id, double price, int64_t ts, SymbolState& s, LayerMode layer) {
        Signal sig;
        sig.symbol = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        sig.layer = (layer == LAYER_IMPULSE) ? LayerType::IMPULSE :
                    (layer == LAYER_EXPANSION) ? LayerType::EXPAND :
                    (layer == LAYER_MICRO) ? LayerType::MICRO : LayerType::LEADLAG;
        sig.expected_bps = (layer == LAYER_MICRO) ? 10.0 :
                          (layer == LAYER_IMPULSE) ? 18.0 :
                          (layer == LAYER_LEADLAG) ? 12.0 : 30.0;
        sig.confidence = 1.0;
        
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
    
    // Set GUI broadcast callback
    void set_gui_broadcast(GuiBroadcastCallback callback) {
        gui_broadcast_ = callback;
    }
    
private:
    void broadcast_to_gui(const std::string& message) {
        std::printf("[BROADCAST-DEBUG] Called, callback is %s\n", gui_broadcast_ ? "SET" : "NULL");
        std::fflush(stdout);
        if (gui_broadcast_) {
            gui_broadcast_(message);
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

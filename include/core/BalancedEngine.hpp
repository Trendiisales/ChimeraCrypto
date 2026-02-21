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
    int direction;  // +1 long, -1 short
    
    // MICRO trailing stop tracking
    double mfe_bps;
    double trail_stop_bps;
    bool trailing_active;

    void reset() {
        state = POS_FLAT;
        entry_price = 0.0;
        entry_ts = 0;
        layer = LAYER_NONE;
        direction = 0;
        mfe_bps = 0.0;
        trail_stop_bps = 0.0;
        trailing_active = false;
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

// Rejection throttling tracker
struct RejectionThrottle {
    std::unordered_map<std::string, int> rejection_counts;
    std::unordered_map<std::string, std::string> last_rejection_reason;
    int64_t last_summary_ts = 0;
    
    void record(const std::string& key, const std::string& reason) {
        rejection_counts[key]++;
        
        // Only log if reason changed
        if (last_rejection_reason[key] != reason) {
            last_rejection_reason[key] = reason;
            std::printf("[SIGNAL-DIAG] %s | reason=%s (will suppress repeats)\n", key.c_str(), reason.c_str());
            std::fflush(stdout);
        }
    }
    
    void print_summary(int64_t ts) {
        if (ts - last_summary_ts < 60000) return; // Every 60s
        
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
    // Callback for broadcasting messages to GUI
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
        
        // Execution mode diagnostic
        std::printf("╔════════════════════════════════════════════════════════════════╗\n");
        std::printf("║         BALANCED ENGINE INITIALIZED - LIVE MODE               ║\n");
        std::printf("╠════════════════════════════════════════════════════════════════╣\n");
        std::printf("║ Signal Diagnostics: THROTTLED (only state changes)           ║\n");
        std::printf("║ Rejection Summary: Every 60s                                  ║\n");
        std::printf("║ PnL Band Tracking: ENABLED                                    ║\n");
        std::printf("║ Engine Stall Detection: ENABLED                               ║\n");
        std::printf("║ Position State Logging: Every 10s                             ║\n");
        std::printf("║ Ready to EXECUTE trades                                       ║\n");
        std::printf("╚════════════════════════════════════════════════════════════════╝\n");
        std::fflush(stdout);
    }

    inline void on_tick(int id, double price, int64_t ts, double latency_ms) {
        // Instrument engine stall detection
        stall_detector_.on_ws_receive();
        stall_detector_.on_eval_start();
        
        // Record evaluation immediately
        std::string my_symbol = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        rejection_telemetry_.recordEvaluation(my_symbol);
        
        // Periodic PnL and stall reports every 60 seconds
        static int64_t last_report_ts = 0;
        if (ts - last_report_ts > 60000) {
            std::printf("%s", pnl_by_band_.generate_report().c_str());
            std::printf("%s", stall_detector_.generate_report().c_str());
            
            // Broadcast rejection summary to GUI BEFORE clearing
            std::printf("[DEBUG] About to broadcast %zu rejection items\n", rejection_throttle_.rejection_counts.size());
            std::fflush(stdout);
            for (const auto& kv : rejection_throttle_.rejection_counts) {
                std::string msg = GuiMessageBuilder::rejection_summary(
                    kv.first,  // key like "BTC MICRO"
                    kv.second, // count
                    rejection_throttle_.last_rejection_reason[kv.first]  // reason
                );
                std::printf("[DEBUG] Broadcasting rejection: %s\n", msg.substr(0, 80).c_str());
                std::fflush(stdout);
                broadcast_to_gui(msg);
            }
            std::printf("[DEBUG] Finished broadcasting rejections\n");
            std::fflush(stdout);
            
            // Print and clear rejection summary
            std::printf("[REJECTION-SUMMARY] Last 60s:\n");
            rejection_throttle_.print_summary(ts);
            
            // Broadcast PnL by latency band to GUI
            std::string pnl_json = pnl_by_band_.build_json();
            broadcast_to_gui("{\"type\":\"latency_band_pnl\",\"data\":" + pnl_json + "}");
            
            // Broadcast engine health (using real stall detector data)
            double event_loop = stall_detector_.max_event_loop_delay_us.load(std::memory_order_relaxed) / 1000.0;
            double eval_dur = stall_detector_.max_eval_duration_us.load(std::memory_order_relaxed) / 1000.0;
            int stalls = static_cast<int>(stall_detector_.stall_events.load(std::memory_order_relaxed));
            int samp = static_cast<int>(stall_detector_.samples.load(std::memory_order_relaxed));
            
            std::printf("[DEBUG] Engine health: loop=%.2fms eval=%.2fms stalls=%d samples=%d\n", 
                       event_loop, eval_dur, stalls, samp);
            std::fflush(stdout);
            
            broadcast_to_gui(GuiMessageBuilder::engine_health(event_loop, eval_dur, stalls, samp));
            
            std::fflush(stdout);
            last_report_ts = ts;
        }
        
        // Position state diagnostic every 10 seconds
        static int64_t last_pos_diag = 0;
        if (ts - last_pos_diag > 10000) {
            std::printf("[POSITION-STATE] open=%d | sys_state=%d | loss_streak=%d | kill_until=%ld\n",
                       open_positions_, system_state_, loss_streak_, 
                       (kill_until_ > ts ? (kill_until_ - ts) : 0));
            
            for (int i = 0; i < 3; ++i) {
                const char* sym = (i == 0) ? "BTC" : (i == 1) ? "ETH" : "SOL";
                std::printf("[SYMBOL-STATE] %s | regime=%d | pos=%s | short_n=%d | long_n=%d | cooldown=%ldms\n",
                           sym, symbols_[i].regime, 
                           (symbols_[i].pos.state == POS_OPEN ? "OPEN" : "FLAT"),
                           symbols_[i].short_n, symbols_[i].long_n,
                           (symbols_[i].cooldown_until > ts ? (symbols_[i].cooldown_until - ts) : 0));
                
                // Broadcast regime updates to GUI
                std::string sym_full = (i == 0) ? "btcusdt" : (i == 1) ? "ethusdt" : "solusdt";
                broadcast_to_gui(GuiMessageBuilder::regime_update(
                    sym_full, symbols_[i].regime, symbols_[i].last_price,
                    symbols_[i].short_n, symbols_[i].short_vol, symbols_[i].long_vol
                ));
            }
            
            // Broadcast performance summary
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
        
        // Update tick counting for velocity
        tick_count_[id]++;
        if (ts - last_tick_count_reset_[id] >= 1000) {
            snapshots_[id].ticks_per_sec = tick_count_[id];
            tick_count_[id] = 0;
            last_tick_count_reset_[id] = ts;
        }
        
        // Update snapshot
        snapshots_[id].last_price = price;
        if (snapshots_[id].ref_price == 0.0) {
            snapshots_[id].ref_price = price;
        }
        snapshots_[id].lat_p95_ms = latency_ms;
        snapshots_[id].spread_bps = 2.0; // TODO: wire real spread
        
        // Update governor
        double vol_score = vol_scoring_[id].get_vol_score();
        governor_.update_volatility(vol_score);
        governor_.update_latency(latency_ms);
        
        // Check if symbol is allowed to trade (every 5 seconds)
        if (ts - last_snapshot_update_[id] >= 5000) {
            last_snapshot_update_[id] = ts;
            
            std::vector<SymbolSnapshot> all_snapshots;
            for (int j = 0; j < 3; ++j) {
                all_snapshots.push_back(snapshots_[j]);
            }
            
            auto active_symbols = allocator_.selectActiveSymbols(all_snapshots);
            
            bool is_active = std::find(active_symbols.begin(), active_symbols.end(), my_symbol) != active_symbols.end();
            
            if (!is_active) {
                rejection_telemetry_.recordBlock(my_symbol, TradeBlockReason::NOT_TOP_RANKED);
                return; // Symbol not in top N, skip trading
            }
        }
        
        rejection_telemetry_.recordEvaluation(my_symbol);
        
        if (ts < kill_until_) return;
        if (latency_ms > 12.0) {
            rejection_telemetry_.recordBlock(my_symbol, TradeBlockReason::LATENCY);
            return;
        }
        if (!latency_gov_.allow_entry(ts)) return;

        SymbolState& s = symbols_[id];

        double move = 0.0;

        if (s.last_price > 0) {
            move = (price - s.last_price) / s.last_price * 10000.0;
            double abs_move = std::fabs(move);

            s.short_vol += abs_move;
            s.long_vol  += abs_move;

            if (s.short_n < 30) s.short_n++;
            if (s.long_n < 240) s.long_n++;

            if (s.short_n == 30) {
                s.short_vol -= s.short_vol / 30.0;
            }
            if (s.long_n == 240) {
                s.long_vol -= s.long_vol / 240.0;
            }
        }

        s.last_price = price;

        if (ts < s.cooldown_until) return;

        // Calculate averages for regime classification
        double short_avg = (s.short_n > 0) ? s.short_vol / s.short_n : 0.0;
        double long_avg = (s.long_n > 0) ? s.long_vol / s.long_n : 0.0;
        s.regime = classify_regime(short_avg, long_avg);

        if (s.pos.state == POS_OPEN) {
            manage_position(id, price, ts, s);
            stall_detector_.on_eval_end();
            return;
        }

        if (open_positions_ > 0) {
            stall_detector_.on_eval_end();
            return;
        }

        if (check_micro(id, price, ts, s, latency_ms)) {
            stall_detector_.on_eval_end();
            return;
        }
        if (check_impulse(id, price, ts, s, latency_ms)) {
            stall_detector_.on_eval_end();
            return;
        }
        if (check_expansion(id, price, ts, s, latency_ms)) {
            stall_detector_.on_eval_end();
            return;
        }
        
        check_leadlag(id, price, ts, s, latency_ms);
        
        stall_detector_.on_eval_end();
    }

    bool check_micro(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
        std::string key = std::string(sym) + " MICRO";
        std::string sym_full = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        
        if (ts < layer_lock_until_) {
            rejection_throttle_.record(key, "layer_lock");
            // NO GUI BROADCAST IN HOT PATH
            return false;
        }
        if (latency_ms > 12.0) {
            rejection_throttle_.record(key, "latency");
            return false;
        }
        if (s.short_n < 20) {
            rejection_throttle_.record(key, "insufficient_samples");
            return false;
        }
        // MICRO allowed in GRIND (1) or BUILDUP (2) - low-vol strategy
        if (s.regime != REGIME_GRIND && s.regime != REGIME_BUILDUP) {
            rejection_throttle_.record(key, "regime_" + std::to_string(s.regime));
            return false;
        }

        double short_avg = s.short_vol / s.short_n;
        double long_avg  = s.long_vol / s.long_n;

        // RELATIVE ONLY — removed absolute threshold (was > 8.0 which is impossible)
        if (short_avg > long_avg * 1.8) {
            std::printf("[SIGNAL-ACCEPT] %s MICRO | rel=%.2f | lat=%.1fms\n",
                       sym, short_avg / long_avg, latency_ms);
            std::fflush(stdout);
            broadcast_to_gui(GuiMessageBuilder::signal_accept(sym_full, "MICRO", short_avg / long_avg));
            layer_lock_until_ = ts + 2000;
            enter(id, price, ts, s, LAYER_MICRO);
            return true;
        }

        rejection_throttle_.record(key, "no_relative_expansion");
        return false;
    }

    bool check_impulse(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
        std::string key = std::string(sym) + " IMPULSE";
        
        if (system_state_ != SYS_IDLE) {
            rejection_throttle_.record(key, "system_state");
            return false;
        }
        if (ts < layer_lock_until_) {
            rejection_throttle_.record(key, "layer_lock");
            return false;
        }
        if (latency_ms > 12.0) {
            rejection_throttle_.record(key, "latency");
            return false;
        }
        if (s.short_n < 25) {
            rejection_throttle_.record(key, "insufficient_samples");
            return false;
        }
        if (s.regime != REGIME_BUILDUP && s.regime != REGIME_BREAKOUT) {
            rejection_throttle_.record(key, "regime_" + std::to_string(s.regime));
            return false;
        }

        double short_avg = s.short_vol / s.short_n;
        double long_avg  = s.long_vol / s.long_n;

        // RELATIVE ONLY — removed absolute threshold (was > 15.0 which is impossible)
        if (short_avg > long_avg * 3.0) {
            std::printf("[SIGNAL-ACCEPT] %s IMPULSE | rel=%.2f | lat=%.1fms\n",
                       sym, short_avg / long_avg, latency_ms);
            std::fflush(stdout);
            system_state_ = SYS_IMPULSE;
            layer_lock_until_ = ts + 5000;
            enter(id, price, ts, s, LAYER_IMPULSE);
            return true;
        }

        rejection_throttle_.record(key, "no_relative_expansion");
        return false;
    }

    bool check_expansion(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
        std::string key = std::string(sym) + " EXPANSION";
        
        if (system_state_ != SYS_IDLE) {
            rejection_throttle_.record(key, "system_state");
            return false;
        }
        if (ts < layer_lock_until_) {
            rejection_throttle_.record(key, "layer_lock");
            return false;
        }
        if (latency_ms > 12.0) {
            rejection_throttle_.record(key, "latency");
            return false;
        }
        if (s.short_n < 30) {
            rejection_throttle_.record(key, "insufficient_samples");
            return false;
        }

        // Expansion allowed in GRIND, BUILDUP, BREAKOUT (not DEAD)
        if (s.regime == REGIME_DEAD) {
            rejection_throttle_.record(key, "regime_DEAD");
            return false;
        }

        double short_avg = s.short_vol / s.short_n;
        double long_avg  = s.long_vol / s.long_n;

        // RELATIVE ONLY — removed absolute threshold (was > 12.0 which is impossible)
        if (short_avg > long_avg * 2.5) {
            std::printf("[SIGNAL-ACCEPT] %s EXPANSION | rel=%.2f | lat=%.1fms\n",
                       sym, short_avg / long_avg, latency_ms);
            std::fflush(stdout);
            system_state_ = SYS_EXPANSION;
            layer_lock_until_ = ts + 8000;

            enter(id, price, ts, s, LAYER_EXPANSION);
            return true;
        }

        rejection_throttle_.record(key, "no_relative_expansion");
        return false;
    }

    bool check_leadlag(int id, double price, int64_t ts, SymbolState& s, double latency_ms) {
        if (ts < layer_lock_until_) return false;
        if (latency_ms > 12.0) return false;

        // LeadLagEngine API check - adjust based on your actual API
        // If your LeadLagEngine doesn't have check_signal(), skip this layer
        // or use the correct method like check_eth_signal() / check_btc_signal()
        
        // Commenting out until you provide correct LeadLagEngine API:
        // auto sig = leadlag_.check_signal(id, ts);
        // if (sig == 0) return false;
        // enter(id, price, ts, s, LAYER_LEADLAG);
        // layer_lock_until_ = ts + 1200;
        
        return false; // Disabled until correct API is provided
    }

    void manage_position(int id, double price, int64_t ts, SymbolState& s) {
        double move = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;
        double pnl_bps = (s.pos.direction > 0) ? move : -move;

        // MICRO: New trailing stop logic
        if (s.pos.layer == LAYER_MICRO) {
            // Hard stop at -3bp
            if (pnl_bps <= -3.0) {
                exit(id, pnl_bps, ts, s);
                return;
            }

            // Track MFE (maximum favorable excursion)
            if (pnl_bps > s.pos.mfe_bps)
                s.pos.mfe_bps = pnl_bps;

            // Activate trailing after +5bp
            if (!s.pos.trailing_active && pnl_bps >= 5.0) {
                s.pos.trailing_active = true;
                s.pos.trail_stop_bps = pnl_bps - 2.0;
            }

            // Update trailing stop (trail by 2bp)
            if (s.pos.trailing_active) {
                double new_stop = pnl_bps - 2.0;
                if (new_stop > s.pos.trail_stop_bps)
                    s.pos.trail_stop_bps = new_stop;

                if (pnl_bps <= s.pos.trail_stop_bps) {
                    exit(id, pnl_bps, ts, s);
                    return;
                }
            }

            // Max hold time safety
            if (ts - s.pos.entry_ts > 3000) {
                exit(id, pnl_bps, ts, s);
            }
            
            return;
        }

        // Other layers: original logic
        double tp = 0, sl = 0;
        int64_t max_hold = 0;

        switch (s.pos.layer) {
            case LAYER_IMPULSE:
                tp = 18; sl = 8; max_hold = 5000; break;
            case LAYER_LEADLAG:
                tp = 12; sl = 6; max_hold = 1200; break;
            case LAYER_EXPANSION: {
                // Trailing stop logic
                expand_peak_price_[id] = std::max(expand_peak_price_[id], price);
                double trail_distance = std::max(0.0012 * s.pos.entry_price, 0.5 * (expand_peak_price_[id] - s.pos.entry_price));
                
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

        if (move >= tp || move <= -sl || ts - s.pos.entry_ts > max_hold) {
            exit(id, move, ts, s);
        }
    }

    void enter(int id, double price, int64_t ts, SymbolState& s, LayerMode layer) {
        // Build signal for governor approval
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
            return; // Governor rejected
        }
        
        // Apply volatility-based sizing
        double vol_score = vol_scoring_[id].get_vol_score();
        double size_mult = vol_scoring_[id].get_size_multiplier(vol_score, latency_gov_.regime() == NET_CLEAN ? 5.0 : 15.0);
        
        // Reduce size on consecutive losses
        if (consecutive_losses_ >= 2) {
            size_mult *= 0.6;
        }
        
        s.pos.state = POS_OPEN;
        s.pos.entry_price = price;
        s.pos.entry_ts = ts;
        s.pos.layer = layer;
        s.pos.direction = 1;  // MICRO is always long (displacement continuation)
        s.pos.mfe_bps = 0.0;
        s.pos.trail_stop_bps = 0.0;
        s.pos.trailing_active = false;

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
        std::printf("[ENTER] %s | layer=%s | regime=%s | px=%.2f | size_mult=%.2f\n", 
            sym, mode, regime_name(s.regime), price, size_mult);
        std::fflush(stdout);
    }

    void exit(int id, double pnl, int64_t ts, SymbolState& s) {
        const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
        std::string symbol_full = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
        
        // Calculate trade metrics
        int64_t hold_time_ms = (ts - s.pos.entry_ts) / 1000;
        double current_latency = snapshots_[id].lat_p95_ms;
        double slippage_bps = 2.0;  // Default estimate - TODO: calculate actual
        
        // Record to PnL by latency band tracker
        pnl_by_band_.record_trade(symbol_full, current_latency, pnl, slippage_bps);
        
        // Track PnL
        realized_pnl_ += pnl;
        total_pnl_ = realized_pnl_;
        total_trades_++;
        
        std::printf("[EXIT] %s | pnl=%.2fbp | lat=%.1fms | hold=%ldms | total_pnl=%.2f | trades=%d\n", 
            sym, pnl, current_latency, hold_time_ms, total_pnl_, total_trades_);
        std::fflush(stdout);

        // Report to governor
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

public:
    std::string get_rejection_stats() const { return rejection_telemetry_.build_json_snapshot(); }
    double get_total_pnl() const { return total_pnl_; }
    double get_realized_pnl() const { return realized_pnl_; }
    int get_total_trades() const { return total_trades_; }
    int get_open_positions() const { return open_positions_; }
    
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
};

}

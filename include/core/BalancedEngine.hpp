#pragma once
#include "LatencyGovernor.hpp"
#include "RegimeClassifier.hpp"
#include "LeadLagEngine.hpp"
#include "VolatilityScoring.hpp"
#include "Governor.hpp"
#include "MultiSymbolAllocator.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>

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

    void reset() {
        state = POS_FLAT;
        entry_price = 0.0;
        entry_ts = 0;
        layer = LAYER_NONE;
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

class BalancedEngine {
public:
    BalancedEngine() : governor_(GovernorConfig()), allocator_(AllocatorConfig()) {
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
    }

    inline void on_tick(int id, double price, int64_t ts, double latency_ms) {
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
            std::string my_symbol = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
            
            bool is_active = std::find(active_symbols.begin(), active_symbols.end(), my_symbol) != active_symbols.end();
            
            if (!is_active) {
                return; // Symbol not in top N, skip trading
            }
        }
        
        if (ts < kill_until_) return;
        if (latency_ms > 12.0) return;
        if (!latency_gov_.allow_entry(ts)) return;

        SymbolState& s = symbols_[id];

        double move = 0.0;

        if (s.last_price > 0) {
            move = (price - s.last_price) / s.last_price * 10000.0;
            double abs_move = std::fabs(move);

            s.short_vol += abs_move;
            s.long_vol  += abs_move;

            if (++s.short_n > 64) s.short_n = 64;
            if (++s.long_n  > 512) s.long_n = 512;
            
            // Classify regime
            if (s.short_n >= 20 && s.long_n >= 100) {
                double short_avg = s.short_vol / s.short_n;
                double long_avg = s.long_vol / s.long_n;
                s.regime = classify_regime(short_avg, long_avg);
            }
        }

        if (ts < s.cooldown_until) {
            s.last_price = price;
            return;
        }

        if (s.pos.state == POS_FLAT) {
            if (open_positions_ >= 2) {
                s.last_price = price;
                return;
            }

            if (system_state_ == SYS_IDLE) {
                // Lead-lag has priority 2 (after expansion)
                if (try_leadlag(id, price, ts, latency_ms, s)) {
                    s.last_price = price;
                    return;
                }
                
                if (try_expansion(id, price, ts, latency_ms, s)) {
                    s.last_price = price;
                    return;
                }
                if (try_impulse(id, price, ts, latency_ms, s)) {
                    s.last_price = price;
                    return;
                }
                if (try_micro(id, price, move, ts, latency_ms, s)) {
                    s.last_price = price;
                    return;
                }
            } else if (system_state_ == SYS_IMPULSE) {
                if (try_impulse(id, price, ts, latency_ms, s)) {
                    s.last_price = price;
                    return;
                }
            } else if (system_state_ == SYS_EXPANSION) {
                s.last_price = price;
                return;
            }
        } else {
            manage_position(id, price, ts, s);
        }
        
        s.last_price = price;
    }

private:

    bool try_leadlag(int id, double price, int64_t ts, double latency_ms, SymbolState& s) {
        if (!latency_gov_.allow_impulse()) return false;
        if (latency_ms > 8.0) return false;
        
        // Only for ETH and SOL
        if (id == 0) return false;
        
        // Check if BTC expansion is active
        bool btc_expand_active = (symbols_[0].pos.state == POS_OPEN && 
                                   symbols_[0].pos.layer == LAYER_EXPANSION);
        
        int direction = 0;
        bool signal = false;
        
        if (id == 1) { // ETH
            signal = leadlag_.check_eth_signal(latency_ms, btc_expand_active, direction);
        } else if (id == 2) { // SOL
            signal = leadlag_.check_sol_signal(latency_ms, btc_expand_active, direction);
        }
        
        if (!signal) return false;
        
        enter(id, price, ts, s, LAYER_LEADLAG);
        return true;
    }

    bool try_micro(int id, double price, double move, int64_t ts, double latency_ms, SymbolState& s) {
        if (!latency_gov_.allow_micro()) return false;
        if (system_state_ != SYS_IDLE) return false;
        if (latency_ms > 10.0) return false;
        if (s.short_n < 20) return false;

        // Micro only in BUILDUP regime
        if (s.regime != REGIME_BUILDUP) return false;

        // Need directional snap for 9-10bp target
        if (std::fabs(move) < 3.5) return false;

        enter(id, price, ts, s, LAYER_MICRO);
        return true;
    }

    bool try_impulse(int id, double price, int64_t ts, double latency_ms, SymbolState& s) {
        if (!latency_gov_.allow_impulse()) return false;
        if (system_state_ == SYS_EXPANSION) return false;
        if (latency_ms > 8.0) return false;
        if (id == 0) return false;

        // Impulse only in BREAKOUT regime
        SymbolState& btc = symbols_[0];
        if (btc.regime != REGIME_BREAKOUT) return false;
        if (btc.short_n < 10) return false;

        double btc_short = btc.short_vol / btc.short_n;
        if (btc_short < 15.0) return false;

        system_state_ = SYS_IMPULSE;
        layer_lock_until_ = ts + 5000;

        enter(id, price, ts, s, LAYER_IMPULSE);
        return true;
    }

    bool try_expansion(int id, double price, int64_t ts, double latency_ms, SymbolState& s) {
        if (system_state_ != SYS_IDLE) return false;
        if (ts < layer_lock_until_) return false;
        if (latency_ms > 12.0) return false;
        if (s.short_n < 30) return false;

        // Expansion allowed in GRIND, BUILDUP, BREAKOUT (not DEAD)
        if (s.regime == REGIME_DEAD) return false;

        double short_avg = s.short_vol / s.short_n;
        double long_avg  = s.long_vol / s.long_n;

        if (short_avg > long_avg * 2.5 && short_avg > 12.0) {
            system_state_ = SYS_EXPANSION;
            layer_lock_until_ = ts + 8000;

            enter(id, price, ts, s, LAYER_EXPANSION);
            return true;
        }

        return false;
    }

    void manage_position(int id, double price, int64_t ts, SymbolState& s) {
        double move = (price - s.pos.entry_price) / s.pos.entry_price * 10000.0;

        double tp = 0, sl = 0;
        int64_t max_hold = 0;

        switch (s.pos.layer) {
            case LAYER_MICRO:
                tp = 10; sl = 5; max_hold = 2000; break;
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
        
        // Track PnL
        realized_pnl_ += pnl;
        total_pnl_ = realized_pnl_;
        total_trades_++;
        
        std::printf("[EXIT] %s | pnl=%.2f | total_pnl=%.2f | trades=%d\n", 
            sym, pnl, total_pnl_, total_trades_);
        std::fflush(stdout);

        // Report to governor
        std::string symbol_full = (id == 0) ? "btcusdt" : (id == 1) ? "ethusdt" : "solusdt";
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
    double get_total_pnl() const { return total_pnl_; }
    double get_realized_pnl() const { return realized_pnl_; }
    int get_total_trades() const { return total_trades_; }
    int get_open_positions() const { return open_positions_; }
    
private:
    SymbolState symbols_[3];
    LatencyGovernor latency_gov_;
    LeadLagEngine leadlag_;
    VolatilityScoring vol_scoring_[3];
    StatefulGovernor governor_;
    MultiSymbolAllocator allocator_;
    
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
};

}

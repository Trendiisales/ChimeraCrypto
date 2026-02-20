#pragma once
#include "SymbolIndex.hpp"
#include "UltraEngine.hpp"
#include "ExecutionStateMachine.hpp"
#include "PnLGovernor.hpp"
#include "ImpulseMatrix.hpp"
#include "MakerQueueModel.hpp"
#include <cmath>
#include <cstdio>

namespace chimera {

struct alignas(64) HybridSymbolState {
    UltraExpansionEngine expansion;
    ExecutionStateMachine exec;
    double last_price;
    double short_vol;
    double long_vol;
    int short_count;
    int long_count;
    bool regime_trending;
    bool regime_compressed;
    int64_t last_regime_flip_ms;

    void reset() {
        last_price = 0.0;
        short_vol = 0.0;
        long_vol = 0.0;
        short_count = 0;
        long_count = 0;
        regime_trending = false;
        regime_compressed = false;
        last_regime_flip_ms = 0;
        exec.reset();
    }
};

class HybridRegimeController {
public:
    HybridRegimeController() {
        for (int i = 0; i < SYM_COUNT; ++i)
            states_[i].reset();
        impulse_.reset();
    }

    inline void on_tick(SymbolId id, double price, int64_t ts, double latency_ms) {
        if (governor_.blocked()) return;
        if (latency_ms > 12.0) return;

        auto& s = states_[id];
        
        // Stricter latency for compression mode
        if (latency_ms > 8.0 && s.regime_compressed) return;

        // Regime update
        if (s.last_price > 0.0) {
            double move = (price - s.last_price) / s.last_price * 10000.0;
            double abs_move = std::fabs(move);

            s.short_vol += abs_move;
            s.long_vol  += abs_move;

            if (++s.short_count > 64) s.short_count = 64;
            if (++s.long_count > 512) s.long_count = 512;

            double short_avg = s.short_vol / s.short_count;
            double long_avg = s.long_vol / s.long_count;

            s.regime_trending = (short_avg > long_avg * 2.0) && (short_avg > 10.0);
            s.regime_compressed = short_avg < 4.5;
        }

        s.last_price = price;
        impulse_.update(id, price, ts);

        // Engine selection
        bool signal = false;

        if (s.regime_trending) {
            s.expansion.on_tick(price, ts, latency_ms);
            signal = s.expansion.signal();
        } else if (s.regime_compressed) {
            double imbalance = 0.5; // TODO: wire real book imbalance
            double spread_bps = 2.0; // TODO: wire real spread
            int64_t time_since_flip = (s.last_regime_flip_ms == 0) ? 9999999 : (ts - s.last_regime_flip_ms);
            signal = compression_signal(id, price, imbalance, spread_bps, s.regime_trending, time_since_flip);
        } else {
            bool impulse_sig = impulse_.follower_signal(0, id, ts);
            signal = impulse_sig;
        }

        auto& x = s.exec;

        // Entry
        if (x.state == FLAT && signal) {
            double imbalance = 0.55;
            double vol = 10.0;

            if (!queue_.should_post(imbalance, vol)) return;

            x.enter(price, ts);

            const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
            const char* mode = s.regime_trending ? "TREND" : s.regime_compressed ? "COMP" : "IMPULSE";
            std::printf("[POST] %s | px=%.2f | mode=%s\n", sym, price, mode);
            std::fflush(stdout);
        }
        // Fill
        else if (x.state == POSTED) {
            x.filled();
        }
        // Exit
        else if (x.state == FILLED) {
            double move = (price - x.entry_price) / x.entry_price * 10000.0;

            double target = s.regime_trending ? 28.0 : s.regime_compressed ? 12.0 : 20.0;
            double stop = s.regime_trending ? 14.0 : s.regime_compressed ? 7.0 : 10.0;

            if (x.should_exit(price, ts, target, stop)) {
                governor_.record(move);

                const char* sym = (id == 0) ? "BTC" : (id == 1) ? "ETH" : "SOL";
                const char* mode = s.regime_trending ? "TREND" : s.regime_compressed ? "COMP" : "IMPULSE";
                std::printf("[EXIT] %s | pnl=%.2f | mode=%s\n", sym, move, mode);
                std::fflush(stdout);

                x.exit();
            }
        } else if (x.state == EXITING) {
            x.flat();
        }
    }

private:
    inline bool compression_signal(
        int id,
        double price,
        double imbalance,
        double spread_bps,
        bool regime_trending,
        int64_t time_since_flip_ms)
    {
        static double last[3] = {0.0, 0.0, 0.0};

        if (regime_trending) return false;
        if (spread_bps > 6.0) return false;
        if (imbalance >= 0.38 && imbalance <= 0.62) return false;
        if (time_since_flip_ms < 500) return false;

        if (last[id] == 0.0) {
            last[id] = price;
            return false;
        }

        double move = (price - last[id]) / last[id] * 10000.0;
        last[id] = price;

        if (std::fabs(move) < 8.0) return false;

        return true;
    }

private:
    HybridSymbolState states_[SYM_COUNT];
    ImpulseMatrix impulse_;
    MakerQueueModel queue_;
    PnLGovernor governor_;
};

}

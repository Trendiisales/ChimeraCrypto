#pragma once
#include "SymbolIndex.hpp"
#include "AlignedEngines.hpp"
#include "UltraEngine.hpp"
#include <cstdio>

namespace chimera {

class UltraController {
public:
    UltraController() {
        for (int i = 0; i < SYM_COUNT; ++i) {
            states_[i].reset();
        }
    }

    inline void on_tick(SymbolId id, double price, int64_t ts, double latency_ms) {
        auto& s = states_[id];
        auto& e = engines_[id];

        if (latency_ms > 12.0) return;

        e.on_tick(price, ts, latency_ms);

        if (!s.in_position) {
            if (ts - s.last_exit_ms < 1500) return;

            if (e.signal()) {
                s.in_position = true;
                s.entry_price = price;
                s.long_side = true;

                const char* sym = (id == SYM_BTC) ? "BTC" : (id == SYM_ETH) ? "ETH" : "SOL";
                std::printf("[ENTRY] %s | px=%.2f | lat=%.2fms\n", sym, price, latency_ms);
                std::fflush(stdout);
            }
        } else {
            double move = (price - s.entry_price) / s.entry_price * 10000.0;

            if (move >= 18.0 || move <= -10.0) {
                s.in_position = false;
                s.last_exit_ms = ts;

                const char* sym = (id == SYM_BTC) ? "BTC" : (id == SYM_ETH) ? "ETH" : "SOL";
                std::printf("[EXIT] %s | px=%.2f | pnl=%.2fbps\n", sym, price, move);
                std::fflush(stdout);
            }
        }
    }

private:
    SymbolState states_[SYM_COUNT];
    UltraExpansionEngine engines_[SYM_COUNT];
};

}

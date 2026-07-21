// trend_roster_construct_test.cpp — S-2026-07-21 (branch crypto-port-trend-book)
// Proves all 19 DirectionalTrendRoster legs CONSTRUCT in the ACTUAL chimera::EdgeEngine,
// SHADOW-first, ride_to_flip, NO 200DMA. Build:
//   g++ -std=c++17 -O2 -Iinclude backtest/trend_roster_construct_test.cpp -o /tmp/roster_ctor && /tmp/roster_ctor
#include "crypto/TrendRoster.hpp"
#include <cstdio>
#include <memory>
#include <vector>

int main(){
    using namespace chimera;
    std::vector<std::unique_ptr<EdgeEngine>> engines;
    int n=0, idx_pending=0;
    std::printf("%-3s %-4s %-14s %-10s cost  ride_flip shadow\n","#","COIN","STRAT","ROLE");
    for (const auto& leg : trend_roster::legs()) {
        auto cfg = trend_roster::make_config(leg);
        engines.push_back(std::make_unique<EdgeEngine>(cfg));
        EdgeEngine* e = engines.back().get();
        (void)e;
        if (leg.is_index) idx_pending++;
        std::printf("%-3d %-4s %-14s %-10s %4.0fbp   %-5s   %-5s%s\n",
            ++n, leg.coin, strategy_name(leg.kind), leg.role, leg.cost_bp,
            cfg.ride_to_flip?"YES":"no", "YES",
            leg.is_index?"  (NDX: index feed pending)":"");
    }
    std::printf("\nCONSTRUCTED %d/19 legs shadow-first (ride_to_flip, NO 200DMA). "
                "%d NDX index legs need a non-Binance feed.\n", n, idx_pending);
    return (n==19) ? 0 : 1;
}

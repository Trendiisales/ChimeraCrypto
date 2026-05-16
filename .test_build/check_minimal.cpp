// Compile-only sanity test for engines that don't depend on libwebsockets
#include "core/FundingWindowEngine.hpp"
#include "core/BasisMomentumEngine.hpp"
#include "core/OrderbookImbalanceEngine.hpp"
int main() {
    chimera::risk::Tier1Risk risk;
    chimera::FundingWindowEngine fw("btcusdt");
    chimera::BasisMomentumEngine bm("ethusdt");
    chimera::OrderbookImbalanceEngine obi("btcusdt");
    fw.set_risk(&risk);
    bm.set_risk(&risk);
    obi.set_risk(&risk);
    return 0;
}

// momentum_factor_cap_test.cpp — Phase-3 item 16.
// XSec / TSMOM / UpJump / RipRider are ONE long-momentum factor. When their
// COMBINED momentum request exceeds the factor cap, every momentum symbol is
// scaled down proportionally (not treated as independent diversification), while
// a non-momentum (mean-reversion) sleeve is left untouched. Also proves the
// sleeve daily-PnL correlation tracker computes.
#include "live/SpotPortfolioAllocator.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
#define NEAR(a,b) (std::fabs((a)-(b)) < 1e-4)

int main() {
    SpotPortfolioAllocator A;
    // momentum cap = $6000; big symbol cap so it doesn't interfere. Only the
    // momentum overlay on (isolate item 16).
    A.configure(/*enforce*/false, /*symbol_cap*/1e9, /*momentum_cap*/6000.0,
                /*target_vol*/0.0, /*cluster_frac*/0.0, /*beta*/0.0);
    A.ref_px = [](const std::string&){ return 100.0; };
    A.set_regime_overlay(false); A.set_drawdown_overlay(false);
    A.set_momentum_overlay(true); A.set_risk_overlay(false);

    // 4 momentum sleeves, distinct coins, $3000 each = $12000 aggregate momentum.
    A.set_target("XSEC",    "BTCUSDT", 3000.0, Factor::MOMENTUM, Family::XSEC);
    A.set_target("TSMOM",   "ETHUSDT", 3000.0, Factor::MOMENTUM, Family::EDGE);
    A.set_target("UPJUMP",  "SOLUSDT", 3000.0, Factor::MOMENTUM, Family::UPJUMP);
    A.set_target("RIP",     "BNBUSDT", 3000.0, Factor::MOMENTUM, Family::RIPRIDER);
    // one mean-reversion sleeve — a DIFFERENT factor, must NOT be capped.
    A.set_target("MREV",    "XRPUSDT", 3000.0, Factor::MEANREV,  Family::EDGE);

    ExchangeLedger L; L.configure(0.0,false,0.0);
    auto d = A.plan(&L);

    // aggregate momentum was $12000 vs $6000 cap -> scale 0.5 on every momentum coin.
    double mom_sum = 0, mrev = 0;
    for (auto& x : d) {
        if (x.symbol == "XRPUSDT") mrev = x.capped_usd;
        else mom_sum += x.capped_usd;
        std::printf("[info] %s capped=$%.2f\n", x.symbol.c_str(), x.capped_usd);
    }
    CHECK(NEAR(mom_sum, 6000.0));           // 4 momentum coins scaled to the cap total
    CHECK(NEAR(mrev, 3000.0));              // mean-reversion sleeve untouched
    // each momentum coin individually halved 3000 -> 1500
    for (auto& x : d) if (x.symbol != "XRPUSDT") CHECK(NEAR(x.capped_usd, 1500.0));

    // ── sleeve daily-PnL correlation tracker computes ───────────────────────
    SleeveCorrelation sc;
    for (int i = 0; i < 10; ++i) {
        double m = (i % 2 == 0) ? 0.02 : -0.01;
        sc.observe("XSEC", m);
        sc.observe("UPJUMP", m);          // identical -> corr ~ +1
        sc.observe("MREV", -m);           // opposite  -> corr ~ -1
    }
    double c_pos = sc.correlation("XSEC","UPJUMP");
    double c_neg = sc.correlation("XSEC","MREV");
    std::printf("[info] corr(XSEC,UPJUMP)=%.3f corr(XSEC,MREV)=%.3f\n", c_pos, c_neg);
    CHECK(c_pos > 0.95);                    // redundant momentum sleeves move together
    CHECK(c_neg < -0.95);

    std::printf(fails==0 ? "PASS: momentum-factor aggregate cap + sleeve correlation\n"
                         : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

// ═══════════════════════════════════════════════════════════════════════════
// campaign_parity_test — CryptoCampaignManager vs the validated backtest
// ═══════════════════════════════════════════════════════════════════════════
// Replays Binance H1 CSVs (ts_ms,o,h,l,c) through the LIVE CryptoCampaignManager
// tick path (open -> low -> close per bar; low-before-close matches the BT's
// stop-on-bar-low-then-close-updates ordering) and compares clip count + net
// against Crypto/backtest/upjump_earlyarm_bt.cpp campaign mode (the harness the
// 4 PASS cells were validated on — CAMPAIGN_LEVERS_2026-07-13.md):
//   UNI-W1 135/270: n=57 +74%   UNI-W2 216/270: n=87 +156%
//   TRX-W8 111/ride: n=35 +92%  LDO-W8 411/342: n=77 +68%
// One cell per manager (per-cell parity; the live UNI book fuses W1+W2 by the
// one-campaign-per-symbol rule, which is a deliberate divergence — the fused
// figure is also printed for reference).
//
// Usage: ./campaign_parity_test <data_dir>   (dir holding UNIUSDT_1h.csv etc.)
#include "core/CryptoCampaignManager.hpp"

#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <ctime>

struct Bar { int64_t ts; double o, h, l, c; };

static std::vector<Bar> load(const std::string& path) {
    std::vector<Bar> v; std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "no %s\n", path.c_str()); return v; }
    std::string ln; std::getline(f, ln);
    while (std::getline(f, ln)) {
        std::stringstream ss(ln); std::string t; std::vector<std::string> c;
        while (std::getline(ss, t, ',')) c.push_back(t);
        if (c.size() < 5) continue;
        v.push_back({(int64_t)std::stoll(c[0]), std::stod(c[1]), std::stod(c[2]),
                     std::stod(c[3]), std::stod(c[4])});
    }
    return v;
}
static int year_of(int64_t ts_ms) {
    time_t s = (time_t)(ts_ms / 1000); struct tm g; gmtime_r(&s, &g);
    return g.tm_year + 1900;
}

// warmup: feed from (first 2023 bar - W - 2) so the ring is warm exactly when
// the BT's from2023 window filter starts admitting entries.
static void run_cell(const char* name, const std::vector<Bar>& bars,
                     chimera::CryptoCampaignManager::CellCfg cc,
                     const std::string& sym, const std::string& pfx,
                     int exp_n, double exp_netpct) {
    chimera::CryptoCostLedger ledger;
    chimera::CryptoOpportunityGate gate;
    chimera::CryptoCampaignManager::Config mc{sym, pfx, 3600, false, {cc}};
    chimera::CryptoCampaignManager mgr(std::move(mc), &ledger, &gate);
    int n = 0; double net = 0.0;
    mgr.set_on_clip([&](const chimera::CryptoCampaignManager::ClipRecord& r) {
        n++; net += r.net_bp;
    });
    size_t start = 0;
    while (start < bars.size() && year_of(bars[start].ts) < 2023) start++;
    start = start > (size_t)(cc.W + 2) ? start - cc.W - 2 : 0;
    for (size_t i = start; i < bars.size(); ++i) {
        const Bar& b = bars[i];
        mgr.on_tick(b.o, b.ts);
        mgr.on_tick(b.l, b.ts + 1);
        mgr.on_tick(b.c, b.ts + 2);
    }
    std::printf("%-12s n=%3d net=%+7.1f%%   (bt: n=%3d net=%+.0f%%)  %s\n",
                name, n, net / 100.0, exp_n, exp_netpct,
                (n == exp_n && std::fabs(net / 100.0 - exp_netpct) < 3.0) ? "MATCH"
                : (std::fabs(net / 100.0 - exp_netpct) < 8.0 ? "close" : "DIVERGENT"));
}

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : "data";
    auto uni = load(dir + "/UNIUSDT_1h.csv");
    auto trx = load(dir + "/TRXUSDT_1h.csv");
    auto ldo = load(dir + "/LDOUSDT_1h.csv");
    if (uni.empty() || trx.empty() || ldo.empty()) return 1;
    run_cell("UNI-W1", uni, {"UNI-CAMP-W1", "CW1-3.5", 1, 0.035, 20.0, 135.0, 270.0, 38.0, 1.0, 0.0, 40.0},
             "uniusdt", "UNI", 57, 74.0);
    run_cell("UNI-W2", uni, {"UNI-CAMP-W2", "CW2-4.0", 2, 0.040, 20.0, 216.0, 270.0, 38.0, 1.0, 0.0, 40.0},
             "uniusdt", "UNI", 87, 156.0);
    run_cell("TRX-W8", trx, {"TRX-CAMP-W8", "CW8-3.5", 8, 0.035, 20.0, 111.0, 0.0, 13.0, 0.5, 0.0, 40.0},
             "trxusdt", "TRX", 35, 92.0);
    run_cell("LDO-W8", ldo, {"LDO-CAMP-W8", "CW8-7.0", 8, 0.070, 20.0, 411.0, 342.0, 48.0, 0.25, 0.0, 40.0},
             "ldousdt", "LDO", 77, 68.0);
    // fused UNI reference (live wiring: one campaign per symbol, W1+W2 detectors)
    {
        chimera::CryptoCostLedger ledger; chimera::CryptoOpportunityGate gate;
        chimera::CryptoCampaignManager::Config mc{"uniusdt", "UNI", 3600, false, {
            {"UNI-CAMP-W1", "CW1-3.5", 1, 0.035, 20.0, 135.0, 270.0, 38.0, 1.0, 0.0, 40.0},
            {"UNI-CAMP-W2", "CW2-4.0", 2, 0.040, 20.0, 216.0, 270.0, 38.0, 1.0, 0.0, 40.0}}};
        chimera::CryptoCampaignManager mgr(std::move(mc), &ledger, &gate);
        int n = 0; double net = 0.0;
        mgr.set_on_clip([&](const chimera::CryptoCampaignManager::ClipRecord& r) { n++; net += r.net_bp; });
        size_t start = 0;
        while (start < uni.size() && year_of(uni[start].ts) < 2023) start++;
        start = start > 4 ? start - 4 : 0;
        for (size_t i = start; i < uni.size(); ++i) {
            mgr.on_tick(uni[i].o, uni[i].ts);
            mgr.on_tick(uni[i].l, uni[i].ts + 1);
            mgr.on_tick(uni[i].c, uni[i].ts + 2);
        }
        std::printf("UNI-FUSED    n=%3d net=%+7.1f%%   (reference — one-campaign-per-symbol mutex, expected < W1+W2 sum)\n",
                    n, net / 100.0);
    }
    return 0;
}

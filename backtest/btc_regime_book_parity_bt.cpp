// btc_regime_book_parity_bt.cpp — S-2026-07-20af
// PARITY: drive the LIVE BtcRegimeMomentumBook engine over the exact 1h CSV the
// python cert used and confirm it reproduces the certified per-sleeve + pooled
// numbers. This is the trust gate: the live book's on_h1_bar() IS the cert's
// run_cell (TRENDCORE) + slowtrend TSMOM30 driver, so identical inputs must give
// identical trades. Any divergence = the live engine is NOT the certified engine.
//
// Build:
//   c++ -std=c++17 -O2 -DBTC_BOOK_STANDALONE -I../include \
//       btc_regime_book_parity_bt.cpp -o /tmp/btc_parity
//   /tmp/btc_parity <btc_1h_csv>
//
// Targets (from backtest/btc_trendcore_2026-07-20/parity_cpp_vs_python.txt +
// gatefix_results.txt PARITY line + T3 ENSEMBLE):
//   orig 2021+ : TRENDCORE n=21 +6813bp PF1.69 | TSMOM30 n=91 +15475bp PF1.71
//                POOLED n=112 +11144bp PF1.70
//   ext  2017+ : TRENDCORE n=37 +18824bp PF2.14 | TSMOM30 n=134 +70931bp PF2.84
//                POOLED n=171 +44878bp PF2.63
#define BTC_BOOK_STANDALONE
#include "core/BtcRegimeMomentumBook.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

using chimera::BtcRegimeMomentumBook;

int main(int argc, char** argv) {
    const char* csv = (argc > 1) ? argv[1]
                                 : "/Users/jo/Crypto/backtest/data/BTCUSDT_1h.csv";
    std::ifstream f(csv);
    if (!f.is_open()) { std::fprintf(stderr, "cannot open %s\n", csv); return 2; }

    // per-sleeve accumulators: n, sum_net_frac, gross_win, gross_loss
    struct Acc { int n = 0; double net = 0, win = 0, loss = 0; } tc, ts;
    BtcRegimeMomentumBook book;
    book.live_enabled = false;   // parity: no live routing, book from on_trade only
    book.on_trade = [&](const BtcRegimeMomentumBook::TradeRec& t) {
        Acc& a = (t.sleeve == BtcRegimeMomentumBook::TRENDCORE) ? tc : ts;
        a.n++; a.net += t.net_frac;
        if (t.net_frac >= 0) a.win += t.net_frac; else a.loss += -t.net_frac;
    };

    std::string line; std::getline(f, line);   // header
    int64_t last_ts = 0; double last_c = 0.0; long rows = 0;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        // CSV: ts_ms,o,h,l,c
        std::stringstream ss(line); std::string cell;
        int64_t ts_ms = 0; double o = 0, h = 0, l = 0, c = 0; int col = 0;
        while (std::getline(ss, cell, ',')) {
            switch (col) {
                case 0: ts_ms = std::atoll(cell.c_str()); break;
                case 1: o = std::atof(cell.c_str()); break;
                case 2: h = std::atof(cell.c_str()); break;
                case 3: l = std::atof(cell.c_str()); break;
                case 4: c = std::atof(cell.c_str()); break;
            }
            ++col;
        }
        if (col < 5) continue;
        int64_t ts = ts_ms / 1000;
        book.on_h1_bar(ts, o, h, l, c);
        last_ts = ts; last_c = c; ++rows;
    }
    book.finalize_stream(last_ts, last_c);

    auto pf = [](const Acc& a) { return a.loss > 0 ? a.win / a.loss : 0.0; };
    Acc pooled; pooled.n = tc.n + ts.n; pooled.net = tc.net + ts.net;
    pooled.win = tc.win + ts.win; pooled.loss = tc.loss + ts.loss;

    std::printf("rows=%ld  last_ts=%lld\n", rows, (long long)last_ts);
    std::printf("  TRENDCORE     : n=%3d net=%+8.0fbp PF=%.2f\n", tc.n, tc.net * 10000.0, pf(tc));
    std::printf("  TSMOM30       : n=%3d net=%+8.0fbp PF=%.2f\n", ts.n, ts.net * 10000.0, pf(ts));
    // POOLED (additive) = both sleeves at full clip = the LIVE book's actual behavior.
    std::printf("  POOLED add    : n=%3d net=%+8.0fbp PF=%.2f  (live: 2 independent clips)\n",
                pooled.n, pooled.net * 10000.0, pf(pooled));
    // POOLED (50/50)   = half capital each = the cert artifact's reporting convention.
    std::printf("  POOLED 50/50  : n=%3d net=%+8.0fbp PF=%.2f  (cert convention)\n",
                pooled.n, pooled.net * 10000.0 / 2.0, pf(pooled));
    return 0;
}

// jf_parity_test — S-2026-07-14: parity check of UpJumpLadderCompanion JUMP-FLOOR
// mode vs Crypto/backtest/upjump2pct_be_bt.cpp percoin cells.
//
// Drives the live class with synthetic ticks derived from the same 1h bars the BT
// used: per bar, ticks at open -> (stop level if low crossed it) -> low -> close.
// The stop-level tick reproduces the BT's resting-stop fill (xp = min(stop, open));
// without it the synthetic low tick would book fills at the bar low (worse than any
// live tick stream — live ticks are dense, the stop fills ~at the stop).
// Expected: same trade count / entries / net as `./upjump2pct_be_bt trades COIN W thr s g`
// (minus the BT's artificial end-of-data flush, which live has no analogue of).
//
// Build: g++ -O2 -std=c++17 -I../include backtest/jf_parity_test.cpp -o /tmp/jf_parity
// Run:   /tmp/jf_parity BTC 1 4.0 0 1.0 /Users/jo/Crypto/backtest/data/BTCUSDT_1h.csv
#include "core/UpJumpLadderCompanion.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 7) { std::fprintf(stderr, "usage: %s COIN W thr_pct s_pct g csv\n", argv[0]); return 1; }
    const int W = atoi(argv[2]);
    const double thr = atof(argv[3]) / 100.0, s = atof(argv[4]) / 100.0, g = atof(argv[5]);

    chimera::UpJumpLadderCompanion::Config c;
    c.parent_tag = std::string(argv[1]) + "-PJ-FEED"; c.tag = std::string(argv[1]) + "-PJ";
    c.symbol = "test"; c.det_w = W; c.det_thr = thr;
    c.jump_floor = true; c.jf_giveback = g; c.jf_prebe_stop_bp = s * 1e4;
    c.tf_secs = 3600; c.round_trip_bp = 20.0;
    c.confirm_bp = 0.0; c.loss_cut_bp = 0.0; c.retire_bp = 0.0;   // retire OFF for parity
    chimera::UpJumpLadderCompanion book(std::move(c));

    int n = 0; double net = 0;
    book.set_on_clip([&](const chimera::UpJumpLadderCompanion::ClipRecord& r) {
        n++; net += r.net_bp_real;
    });

    std::ifstream f(argv[6]); std::string ln; std::getline(f, ln);
    long bars = 0;
    while (std::getline(f, ln)) {
        std::stringstream ss(ln); std::string t; std::vector<std::string> v;
        while (std::getline(ss, t, ',')) v.push_back(t);
        if (v.size() < 5) continue;
        const int64_t ts = (int64_t)std::stoll(v[0]) * 1000;   // csv ts in seconds
        const double o = std::stod(v[1]), l = std::stod(v[3]), cl = std::stod(v[4]);
        // open tick FIRST: finalizes the prior bar's close in the aggregator (fills
        // pending open/exit at this bar's open == the BT's "next open"), and doubles
        // as the gap-through stop fill (px = open when open already through the stop).
        book.observe(false, 0, o,  ts + 1);
        // descend open -> low in fine steps: a resting stop fills within one step of
        // its level (the BT fills exactly AT the stop; live ticks are dense so this
        // mirrors reality — 128 steps keeps the discretization under ~1bp/trade).
        const int STEPS = 128;
        for (int k = 1; k <= STEPS; ++k)
            book.observe(false, 0, o + (l - o) * k / STEPS, ts + 1 + k * 20000);
        book.observe(false, 0, cl, ts + 3599000);   // running close (finalized by next bar's open tick)
        bars++;
    }
    std::printf("PARITY %s W=%d thr=%.1f%% s=%.1f%% g=%.1f | bars=%ld clips=%d net=%+.0fbp (BT ref minus end-flush)\n",
        argv[1], W, thr * 100, s * 100, g, bars, n, net);
    return 0;
}

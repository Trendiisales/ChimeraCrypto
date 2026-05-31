// HARNESS GROUND-TRUTH (S54). Question: does the harness's intra-bar feed value a
// WINNING trade correctly, or does the low-first feed stop trades out on normal
// pullbacks (over-pessimism)? Feed a synthetic +60% uptrend with realistic
// intra-bar dips, using the EXACT feed order run_backtest uses (o, low, high,
// close), and assert a simple long captures most of the move.
//
// If a long-only TSMOM CANNOT profit on a clean +60% trend with sane stops, the
// harness feed is broken — NOT the strategy.
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <vector>
#include <cmath>

using chimera::EdgeEngine;

int main() {
    printf("=== HARNESS GROUND-TRUTH: can a long capture a +60%% trend? ===\n\n");

    // Build 200 daily bars: close rises ~0.4%/day (+123% over 200d) with a
    // realistic intra-bar dip each bar (low 1.2% below close, high 0.4% above).
    struct Bar { double o,h,l,c; };
    std::vector<Bar> bars;
    double px = 100.0;
    for (int i = 0; i < 200; ++i) {
        double open = px;
        double close = px * 1.004;          // +0.4%/day uptrend
        double high = close * 1.004;
        double low  = open  * 0.988;         // 1.2% intra-bar dip below the open
        bars.push_back({open, high, low, close});
        px = close;
    }
    printf("synthetic trend: %.1f -> %.1f (%+.0f%%), 1.2%%/bar intra-dip\n\n",
           bars.front().o, bars.back().c, 100.0*(bars.back().c/bars.front().o - 1.0));

    EdgeEngine::Config c;
    c.symbol = "btcusdt"; c.tag = "TRUTH"; c.kind = chimera::StrategyKind::TSMOM;
    c.tf_secs = 86400; c.lookback = 10; c.hold_bars = 50; c.atr_period = 14;
    c.sl_atr_mult = 2.5; c.round_trip_bp = 20.0; c.max_history = 64;
    c.trail_arm_atr = 1.0; c.trail_dist_atr = 0.5;
    EdgeEngine e(c);
    e.apply_safety_preset();
    e.set_hard_floor_bp(-170.0);
    e.set_ratchet_start_bp(8.0);
    e.set_be_arm_bp(15.0);
    e.set_realistic_gap_fill(false);

    // Seed warmup so signals can fire.
    std::vector<EdgeEngine::SeedBar> seed;
    for (int i = 0; i < 30; ++i) {
        EdgeEngine::SeedBar b{};
        b.open_ts_ms = (int64_t)i*86400000; b.o=bars[i].o; b.h=bars[i].h; b.l=bars[i].l; b.c=bars[i].c;
        seed.push_back(b);
    }
    e.seed_bars(seed);

    // Feed remaining bars using the harness's EXACT order: open, LOW, high, close.
    int64_t t = 30LL*86400000;
    int64_t step = 86400000/4;
    for (int i = 30; i < (int)bars.size(); ++i) {
        const Bar& b = bars[i];
        e.on_tick(b.o, t);
        e.on_tick(b.l, t + step);       // LOW first (the suspect)
        e.on_tick(b.h, t + step*2);
        e.on_tick(b.c, t + step*3);
        t += 86400000;
    }
    e.graceful_close(bars.back().c, t);

    double net = e.total_bp();
    int trades = e.trades(), wins = e.wins();
    printf("RESULT: net=%+.0fbp  trades=%d  wins=%d  WR=%.0f%%\n",
           net, trades, wins, trades? 100.0*wins/trades : 0.0);

    int failures = 0;
    printf("\n");
    if (net > 1000.0)
        printf("[PASS] long captured the trend (net %+.0fbp > +1000) — harness feed is HONEST\n", net);
    else {
        printf("[FAIL] long LOST/under-captured a +123%% trend (net %+.0fbp) — feed is OVER-PESSIMISTIC\n", net);
        printf("       -> low-first intra-bar feed is stopping trades out on normal pullbacks\n");
        failures++;
    }
    printf("\n%s\n", failures ? "HARNESS BROKEN" : "HARNESS OK");
    return failures;
}

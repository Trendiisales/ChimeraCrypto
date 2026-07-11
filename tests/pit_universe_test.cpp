// Phase-5 item 27 — POINT-IN-TIME universe = NO look-ahead.
// (a) a coin is ineligible before it has min_history_days of data (listing-age);
// (b) eligibility at index i is invariant to data added AFTER i (no future leak);
// (c) an illiquid coin is excluded (dynamic liquidity gate, not a survivor list);
// (d) streaming (on_tick) parity — the same eligibility the batch sim sees.
#include "core/CrossSectionalMomentum2Engine.hpp"
#include <cstdio>
#include <cmath>

using namespace chimera;

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    XSec2Config cfg; cfg.min_history_days = 120; cfg.liq_window = 30;
    cfg.min_dollar_vol = 2.0e6;
    CrossSectionalMomentum2Engine e(cfg);
    std::vector<std::string> U = {"OLD","NEW","ILLIQ"};
    e.set_universe(U);

    int N = 400;
    for (int d = 0; d < N; ++d) {
        double px = 100.0 * std::pow(1.003, d);
        e.seed_daily("OLD", d, px, 5e7);                 // listed from day 0, liquid
        if (d >= 250) e.seed_daily("NEW", d, px, 5e7);    // lists on day 250
        e.seed_daily("ILLIQ", d, px, 1.0e5);              // liquid price, tiny $-vol
    }

    // find dense-axis indices — engine keys on the day value directly.
    // (a) listing-age: NEW not eligible at day 300 (only 50 days of history)
    ck( e.eligible("OLD",  300), "OLD eligible at day 300 (long history + liquid)");
    ck(!e.eligible("NEW",  300), "NEW NOT eligible at day 300 (<120d since listing)");
    ck( e.eligible("NEW",  399), "NEW eligible at day 399 (>=120d since listing)");
    // (c) liquidity gate excludes ILLIQ regardless of history
    ck(!e.eligible("ILLIQ",300), "ILLIQ excluded by the $-volume gate (not a survivor list)");

    // (b) NO look-ahead: eligibility at an EARLIER index must not change when we
    //     append MORE future data. Snapshot NEW@day300, then add more days.
    bool new_at_300_before = e.eligible("NEW", 300);
    for (int d = N; d < N + 100; ++d)
        for (auto& s : U) e.seed_daily(s, d, 100.0*std::pow(1.003, d), 5e7);
    bool new_at_300_after = e.eligible("NEW", 300);
    ck(new_at_300_before == new_at_300_after,
       "eligibility at day 300 unchanged after appending future data (point-in-time)");

    // (d) streaming parity: a fresh engine fed the SAME closes via on_tick day
    //     rollovers must reach the same eligibility decision at the last day.
    //     on_tick has no volume, so isolate the day-axis/history logic from the
    //     liquidity gate (min_dollar_vol=0) — that's what parity is about here.
    XSec2Config cfg2 = cfg; cfg2.min_dollar_vol = 0.0;
    CrossSectionalMomentum2Engine bt(cfg2), st(cfg2);
    bt.set_universe({"X"}); st.set_universe({"X"});
    for (int d = 0; d < 200; ++d) {
        double px = 50.0 * std::pow(1.004, d);
        bt.seed_daily("X", d, px, 5e7);
        // stream: two ticks per day then roll by crossing the UTC day boundary
        st.on_tick("X", px, (int64_t)d*86400000LL + 3600000LL);
        st.on_tick("X", px, (int64_t)(d+1)*86400000LL + 1000LL);   // rolls day d
    }
    // both have >=120 days of history for X now
    bool bte = bt.eligible("X", 190);
    // stream engine's last dense index:
    size_t sti = st.num_days() ? st.num_days()-1 : 0;
    bool ste = st.eligible("X", sti > 190 ? 190 : sti);
    printf("  streaming parity: batch elig(X@190)=%d stream elig=%d (stream days=%zu)\n",
           bte, ste, st.num_days());
    ck(bte == ste, "streaming eligibility matches batch");

    if (fails == 0) { printf("PASS pit_universe_test\n"); return 0; }
    printf("FAIL pit_universe_test (%d)\n", fails); return 1;
}

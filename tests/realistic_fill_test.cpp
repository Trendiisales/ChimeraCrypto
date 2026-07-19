// Phase-4 item 22 — shadow-fill realism (ADDITIVE parallel metric).
// Tests: (a) a realistic fill is strictly worse than the signal price (buy
// pays up, sell receives down, taker fee charged); (b) over a round-trip the
// realistic book PnL <= the signal-price book PnL (costs only subtract); (c)
// the comparator is a side metric — it exposes the modelled-cost gap without
// any hook into the real shadow ledger or the grid (nothing to disturb).
#include "live/RealisticFill.hpp"
#include <cstdio>

using namespace chimera;

int main() {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) { std::printf("  FAIL: %s\n", msg); failures++; }
    };

    FillModelParams p;
    p.half_spread_bp = 2.0; p.slippage_bp = 1.0; p.taker_fee_bp = 10.0;
    p.queue_fill_ratio = 1.0;

    // (a) buy fills UP, sell fills DOWN, fee charged
    RealisticFillModel m; m.configure(p);
    RealisticFill fb = m.apply(/*buy*/true, 100.0, 10.0);
    RealisticFill fs = m.apply(/*buy*/false, 100.0, 10.0);
    check(fb.fill_px > 100.0, "buy fills above signal (spread+slippage)");
    check(fs.fill_px < 100.0, "sell fills below signal");
    check(fb.fee_quote > 0.0 && fs.fee_quote > 0.0, "taker fee charged both sides");
    check(fb.cost_bp > 0.0, "buy carries a positive cost vs signal");
    std::printf("  (a) buy fill=%.4f(+%.1fbp) sell fill=%.4f fee=%.4f\n",
                fb.fill_px, fb.cost_bp, fs.fill_px, fb.fee_quote);

    // (b) round-trip: buy 100 -> sell 110 (a winner). Realistic book keeps LESS.
    ShadowFillComparator cmp; cmp.configure(p);
    cmp.on_fill("MIMIC", "solusdt", /*buy*/true,  100.0, 10.0);
    cmp.on_fill("MIMIC", "solusdt", /*buy*/false, 110.0, 10.0);
    double sig = cmp.signal_book_pnl();
    double real = cmp.realistic_book_pnl();
    check(sig > 0.0, "signal book realized the winner (+100)");
    check(real <= sig + 1e-9, "realistic book PnL <= signal book PnL (costs modelled)");
    check(real < sig, "realistic book strictly lower (spread+slippage+fees bite)");
    check(cmp.total_modelled_cost() > 0.0, "modelled-cost gap is positive");
    std::printf("  (b) signal_pnl=%.2f realistic_pnl=%.2f modelled_cost=%.2f\n",
                sig, real, cmp.total_modelled_cost());

    // (c) partial/queue fill: only part of the requested qty fills
    FillModelParams pq = p; pq.queue_fill_ratio = 0.5;
    RealisticFillModel mq; mq.configure(pq);
    RealisticFill fq = mq.apply(true, 100.0, 10.0);
    check(std::fabs(fq.filled_qty - 5.0) < 1e-9, "queue partial fills only 50% of qty");
    std::printf("  (c) partial fill qty=%.2f of 10 requested\n", fq.filled_qty);

    if (failures == 0) { std::printf("PASS realistic_fill_test\n"); return 0; }
    std::printf("FAIL realistic_fill_test (%d)\n", failures); return 1;
}

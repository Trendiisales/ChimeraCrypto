// storm_guards_test.cpp — ADVERSARIAL verification of the S-2026-07-27p storm guards
// ported from Omega (`feedback-safety-fix-both-systems-default`).
//
// Standard applied (Omega CLAUDE.md § "Every Gate Is Adversarially Verified"):
//
//     A gate nobody has watched FAIL is not a gate. It is a comment that compiles.
//
// So every gate below is (1) driven with an input constructed to TRIP it and the
// literal refusal printed, (2) boundary-walked -- n-1 must PASS and n+1 must FAIL --
// and (3) tested against a legitimate risk-REDUCING close, which must still get out.
//
// NOTHING HERE TOUCHES THE EXCHANGE. The gateway is constructed over a MockExec whose
// execute() is a local function; `BinanceREST` is never instantiated, no socket is
// opened, and no key is loaded. The live crypto desk is STOPPED and DISABLED by the
// operator and is not contacted by this file.
//
// Build:
//   g++ -std=c++20 -I../include -I<curl>/include -I<openssl>/include \
//       storm_guards_test.cpp -o storm_guards_test
#include "live/ExecutionGateway.hpp"
#include <cstdio>
#include <string>
using namespace chimera;

// Mock executor with a PUBLIC execute(). `fill` controls whether the order comes back
// filled -- the difference between a FILLING storm and a non-filling retry loop, which
// are bounded by two DIFFERENT gates and must be tested separately.
struct MockExec {
    int    calls = 0;
    bool   fill  = true;
    std::string status_when_unfilled = "CANCELED";
    OrderResult execute(const std::string&, bool, double qty, double px,
                        const std::string& = "") {
        ++calls;
        OrderResult r;
        r.ok = true;
        r.shadow = false;
        if (fill) { r.status = "FILLED"; r.executed_qty = qty; r.avg_price = px; }
        else      { r.status = status_when_unfilled; r.executed_qty = 0.0; r.avg_price = 0.0; }
        return r;
    }
};

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } \
                     else { std::printf("  ok: %s\n", #c); } }while(0)

static OrderIntent entry(const char* sym, double qty = 1.0, double px = 100.0) {
    OrderIntent in; in.symbol = sym; in.is_buy = true; in.qty = qty; in.ref_px = px;
    in.is_exit = false; in.source = "TEST"; return in;
}
static OrderIntent exit_of(const char* sym, double qty = 1.0, double px = 100.0) {
    OrderIntent in; in.symbol = sym; in.is_buy = false; in.qty = qty; in.ref_px = px;
    in.is_exit = true; in.source = "TEST"; return in;
}

int main() {
    // ══════════════════════════════════════════════════════════════════════════
    // GATE A — DESK-WIDE SLIDING RATE (GlobalOrderRateGuard, 1s window)
    //
    // The defect it replaces, measured in ExecutionGateway.hpp before this change:
    //     if (now_cb - rate_win_start_ms_ >= 1000) { rate_win_start_ms_ = now_cb;
    //                                                rate_win_count_ = 0; }
    // -- a bucket that zeroes itself every second. Cap was 25/sec.
    // ══════════════════════════════════════════════════════════════════════════
    std::printf("\n=== GATE A: desk-wide SLIDING 1s rate cap -- BOUNDARY (8 pass / 9th fails) ===\n");
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        gw.rate_guard().max_orders_per_sec_ = 8;
        gw.rate_guard().max_new_syms_per_min_ = 1000;   // isolate: fan-out not under test
        int sent = 0, blocked = 0;
        std::string first_block;
        for (int i = 0; i < 9; ++i) {
            auto r = gw.submit(entry("BTCUSDT"));
            if (r.ok) ++sent;
            else { ++blocked; if (first_block.empty()) first_block = r.error; }
        }
        std::printf("  9 same-second entries -> sent=%d blocked=%d\n", sent, blocked);
        std::printf("  literal refusal: %s\n", first_block.c_str());
        CHECK(sent == 8);                       // n-1 (and n) pass
        CHECK(blocked == 1);                    // n+1 fails
        CHECK(first_block.find("orders in 1s") != std::string::npos);
    }

    std::printf("\n=== GATE A: a FILLING storm cannot reset it (the QQQ-240..251 shape) ===\n");
    {
        // This is the case the per-symbol unfilled counter is STRUCTURALLY blind to,
        // because every fill zeroes it. The sliding window has no such reset.
        MockExec ex; ex.fill = true;
        ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        gw.rate_guard().max_orders_per_sec_ = 8;
        gw.rate_guard().max_new_syms_per_min_ = 1000;
        int sent = 0;
        for (int i = 0; i < 12; ++i) if (gw.submit(entry("BTCUSDT")).ok) ++sent;
        std::printf("  12 entries, EVERY ONE FILLED -> %d reached the executor\n", sent);
        CHECK(sent == 8);          // 12 unbounded before; 8 now
        CHECK(ex.calls == 8);
    }

    std::printf("\n=== GATE A: INVERSE -- an EXIT is counted but NEVER blocked ===\n");
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        gw.rate_guard().max_orders_per_sec_ = 2;
        gw.rate_guard().max_new_syms_per_min_ = 1000;
        // Blow the cap wide open with entries first.
        for (int i = 0; i < 20; ++i) gw.submit(entry("BTCUSDT"));
        int before = ex.calls;
        auto r = gw.submit(exit_of("BTCUSDT"));
        std::printf("  rate cap 2/s, 20 entries already refused; now an EXIT -> ok=%d\n", (int)r.ok);
        CHECK(r.ok);                            // the close MUST get out
        CHECK(ex.calls == before + 1);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // GATE B — NEW-SYMBOL FAN-OUT CAP (the control that did NOT exist anywhere)
    // Six symbols each firing exactly ONE order is unremarkable to every per-symbol
    // control. Only a desk-wide view sees it.
    // ══════════════════════════════════════════════════════════════════════════
    std::printf("\n=== GATE B: new-symbol FAN-OUT -- BOUNDARY (4 pass / 5th and 6th fail) ===\n");
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        gw.rate_guard().max_orders_per_sec_ = 1000;      // isolate: rate not under test
        gw.rate_guard().max_new_syms_per_min_ = 4;
        const char* syms[6] = { "BTCUSDT","ETHUSDT","SOLUSDT","XRPUSDT","ADAUSDT","TRXUSDT" };
        int ok = 0; std::string last_err;
        for (int i = 0; i < 6; ++i) {
            // ledger_ is null here, so `new_symbol` is computed as false by the gateway.
            // Drive the guard directly so the fan-out logic itself is what is tested --
            // otherwise this would silently pass for the wrong reason.
            auto v = gw.rate_guard().admit(syms[i], /*risk_reducing*/false, /*new_symbol*/true);
            if (v.allow) ++ok; else last_err = v.why;
            std::printf("  %-8s allow=%d %s\n", syms[i], (int)v.allow, v.why.c_str());
        }
        CHECK(ok == 4);
        CHECK(last_err.find("FAN-OUT") != std::string::npos);
    }

    std::printf("\n=== GATE B: INVERSE -- a risk-reducing order is exempt from the fan-out cap ===\n");
    {
        GlobalOrderRateGuard g;
        g.max_new_syms_per_min_ = 1;
        g.admit("BTCUSDT", false, true);                       // burn the only slot
        auto v = g.admit("ETHUSDT", /*risk_reducing*/true, /*new_symbol*/true);
        std::printf("  fan-out cap 1, already used; risk-reducing ETHUSDT -> allow=%d\n", (int)v.allow);
        CHECK(v.allow);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // GATE C — CUMULATIVE UNFILLED PER SYMBOL PER UTC DAY (SlowStormGuard)
    // The slow drip: ~18 sends spread over 30 minutes, zero fills. A 30-SECOND
    // window breaker never sees 8 of them at once.
    // ══════════════════════════════════════════════════════════════════════════
    std::printf("\n=== GATE C: cumulative unfilled/day -- BOUNDARY (12 pass / 13th fails) ===\n");
    {
        SlowStormGuard s;
        s.max_unfilled_per_sym_day_ = 12;
        s.max_consecutive_ = 100000;          // isolate: backoff not under test here
        int allowed = 0; std::string why;
        for (int i = 0; i < 13; ++i) {
            auto v = s.admit("BTCUSDT", "OPEN", false, SlowStormGuard::AUTOMATED);
            if (v.allow) { ++allowed; s.on_send("BTCUSDT"); }
            else why = v.why;
        }
        std::printf("  13 unfilled attempts (no burst window involved) -> allowed=%d\n", allowed);
        std::printf("  literal refusal: %s\n", why.c_str());
        CHECK(allowed == 12);
        CHECK(why.find("unfilled orders today") != std::string::npos);
    }

    std::printf("\n=== GATE C: a CANCEL does NOT reset the counter (a fill does) ===\n");
    {
        SlowStormGuard s;
        s.max_unfilled_per_sym_day_ = 3;
        s.max_consecutive_ = 100000;
        for (int i = 0; i < 3; ++i) { s.admit("ETHUSDT","OPEN",false,SlowStormGuard::AUTOMATED);
                                      s.on_send("ETHUSDT"); s.on_fail("ETHUSDT","OPEN"); }
        auto blocked = s.admit("ETHUSDT", "OPEN", false, SlowStormGuard::AUTOMATED);
        std::printf("  after 3 sends all CANCELLED -> allow=%d (%s)\n",
                    (int)blocked.allow, blocked.why.c_str());
        CHECK(!blocked.allow);                       // cancels did not erase the evidence
        s.on_fill("ETHUSDT");
        auto after_fill = s.admit("ETHUSDT", "OPEN", false, SlowStormGuard::AUTOMATED);
        std::printf("  after ONE real fill        -> allow=%d\n", (int)after_fill.allow);
        CHECK(after_fill.allow);                     // a filling symbol is trading
    }

    std::printf("\n=== GATE C: INVERSE -- an OPERATOR close is never refused, in any state ===\n");
    {
        SlowStormGuard s;
        s.max_unfilled_per_sym_day_ = 1;
        s.max_consecutive_ = 1;
        for (int i = 0; i < 40; ++i) { s.on_send("BTCUSDT"); s.on_fail("BTCUSDT","EXIT"); }
        auto autoc = s.admit("BTCUSDT","EXIT",/*risk_reducing*/true, SlowStormGuard::AUTOMATED);
        auto human = s.admit("BTCUSDT","EXIT",/*risk_reducing*/true, SlowStormGuard::OPERATOR);
        std::printf("  40 failures; AUTOMATED close allow=%d (%s) ; OPERATOR close allow=%d\n",
                    (int)autoc.allow, autoc.why.c_str(), (int)human.allow);
        CHECK(human.allow);        // the 2026-07-27 Omega failure, made impossible
        // CORRECTION, recorded rather than quietly edited away: the first cut of this
        // test asserted the AUTOMATED close also passes, and it FAILED. That assertion
        // was wrong, not the gate. An automated risk-reducing RETRY is deliberately
        // BACKED OFF -- it is the loop -- but never permanently stopped: the STOPPED
        // branch exempts risk_reducing, and the backoff is capped at 1h, so the same
        // automated close is admitted again once the delay elapses. "Slowed, never
        // stopped" for automation; "never delayed at all" only for the operator.
        CHECK(!autoc.allow);
        CHECK(autoc.why.find("backing off") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // GATE D — EXPONENTIAL BACKOFF + STOP AFTER N CONSECUTIVE FAILURES
    // ══════════════════════════════════════════════════════════════════════════
    std::printf("\n=== GATE D: backoff -- BOUNDARY (5 failures = backoff only / 6th = STOPPED) ===\n");
    {
        SlowStormGuard s;
        s.max_consecutive_ = 6;
        s.max_unfilled_per_sym_day_ = 100000;   // isolate
        s.backoff_base_ms_ = 0;                 // remove the time term; test the STOP latch
        s.backoff_cap_ms_  = 0;
        for (int i = 0; i < 5; ++i) s.on_fail("SOLUSDT","OPEN");
        auto at5 = s.admit("SOLUSDT","OPEN",false,SlowStormGuard::AUTOMATED);
        s.on_fail("SOLUSDT","OPEN");
        auto at6 = s.admit("SOLUSDT","OPEN",false,SlowStormGuard::AUTOMATED);
        std::printf("  after 5 consecutive failures -> allow=%d\n", (int)at5.allow);
        std::printf("  after 6 consecutive failures -> allow=%d (%s)\n",
                    (int)at6.allow, at6.why.c_str());
        CHECK(at5.allow);                                   // n-1 passes
        CHECK(!at6.allow);                                  // n+1 fails
        CHECK(at6.why.find("stopped after") != std::string::npos);
    }

    std::printf("\n=== GATE D: backoff is TIME-based and geometric, not a fixed skip ===\n");
    {
        SlowStormGuard s;
        s.max_consecutive_ = 100000;
        s.max_unfilled_per_sym_day_ = 100000;
        s.backoff_base_ms_ = 60000;
        s.on_fail("XRPUSDT","OPEN");
        auto v1 = s.admit("XRPUSDT","OPEN",false,SlowStormGuard::AUTOMATED);
        std::printf("  after 1 failure -> allow=%d (%s)\n", (int)v1.allow, v1.why.c_str());
        CHECK(!v1.allow);
        CHECK(v1.why.find("backing off") != std::string::npos);
        s.on_fill("XRPUSDT");                               // a fill clears the latch
        auto v2 = s.admit("XRPUSDT","OPEN",false,SlowStormGuard::AUTOMATED);
        std::printf("  after a FILL    -> allow=%d\n", (int)v2.allow);
        CHECK(v2.allow);
    }

    std::printf("\n=== GATE D: INVERSE -- a STOPPED symbol still lets a risk-reducing order out ===\n");
    {
        SlowStormGuard s;
        s.max_consecutive_ = 2;
        s.max_unfilled_per_sym_day_ = 100000;
        s.backoff_base_ms_ = 0; s.backoff_cap_ms_ = 0;
        for (int i = 0; i < 5; ++i) s.on_fail("SOLUSDT","EXIT");
        auto v = s.admit("SOLUSDT","EXIT",/*risk_reducing*/true, SlowStormGuard::AUTOMATED);
        std::printf("  STOPPED after 5 failures; automated EXIT -> allow=%d\n", (int)v.allow);
        CHECK(v.allow);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // GATE E — THE END-TO-END SLOW DRIP THROUGH THE REAL submit() CHOKEPOINT
    // Not the guard in isolation: the actual gateway, with unfilled results coming
    // back, which is what the live loop looks like.
    // ══════════════════════════════════════════════════════════════════════════
    std::printf("\n=== GATE E: 20 non-filling entries through the REAL submit() ===\n");
    {
        MockExec ex; ex.fill = false; ex.status_when_unfilled = "CANCELED";
        ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        gw.rate_guard().max_orders_per_sec_ = 100000;    // isolate: the burst is not the point
        gw.rate_guard().max_orders_per_min_ = 100000;
        gw.rate_guard().max_new_syms_per_min_ = 100000;
        gw.slow_storm().max_consecutive_ = 100000;       // isolate: cumulative counter only
        // CORRECTION, recorded not hidden: the first cut isolated max_consecutive_ but
        // NOT the backoff, so only ONE order reached the executor and the result would
        // have been credited to the cumulative counter when the BACKOFF is what bound
        // it -- the same mis-attribution the Omega verify caught in its case (c). Zero
        // the backoff so the cumulative counter is provably the binding control here.
        // Its real-world effect is shown separately in GATE E2 below.
        gw.slow_storm().backoff_base_ms_ = 0;
        gw.slow_storm().backoff_cap_ms_  = 0;
        gw.slow_storm().max_unfilled_per_sym_day_ = 12;
        int reached = 0;
        for (int i = 0; i < 20; ++i) if (gw.submit(entry("ADAUSDT")).ok) ++reached;
        std::printf("  20 attempts, every one CANCELLED unfilled -> %d reached the executor\n",
                    ex.calls);
        // ATTRIBUTION, corrected mid-run and recorded rather than tuned away. The first
        // cut asserted 12 and measured 8 -- because through the REAL chokepoint the
        // PRE-EXISTING per-symbol breaker (MAX_UNFILLED_PER_SYM = 8, a static constexpr
        // that cannot be raised from a test) trips at attempt 9, before the new
        // cumulative counter's 12 is ever reached. Crediting the new gate for that
        // result would be exactly the self-deception this file exists to prevent.
        // What this case therefore proves: through the real submit(), the composite
        // bound on a non-filling drip is 8, and the sticky circuit trips. The new
        // cumulative counter's OWN boundary (12/13) is proven in isolation in GATE C,
        // and its real production effect in GATE E2.
        CHECK(ex.calls == 8);
        CHECK(reached == 8);
        // INVERSE, and the most important assertion in this file: the sticky circuit
        // breaker is now TRIPPED on this gateway, and an exit must still get out.
        auto rr = gw.submit(exit_of("ADAUSDT"));
        std::printf("  ...circuit TRIPPED; an EXIT on the SAME symbol -> ok=%d\n", (int)rr.ok);
        CHECK(rr.ok);
        OrderIntent blocked_entry = entry("ADAUSDT");
        auto br = gw.submit(blocked_entry);
        std::printf("  ...and a fresh ENTRY is refused -> ok=%d (%s)\n",
                    (int)br.ok, br.error.c_str());
        CHECK(!br.ok);
    }

    std::printf("\n=== GATE E2: at PRODUCTION defaults the drip stops at the FIRST failure ===\n");
    {
        // Nothing isolated. This is what the live gateway actually does to the
        // 2026-07-27 shape (~18 sends, all cancelled, spread over 30 minutes): the
        // very first cancel arms a 60s geometric backoff, so attempt 2 never leaves.
        MockExec ex; ex.fill = false; ex.status_when_unfilled = "CANCELED";
        ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        gw.rate_guard().max_orders_per_sec_ = 100000;
        gw.rate_guard().max_orders_per_min_ = 100000;
        for (int i = 0; i < 18; ++i) gw.submit(entry("DOTUSDT"));
        std::printf("  18 attempts at DEFAULT caps -> %d reached the executor\n", ex.calls);
        CHECK(ex.calls == 1);
        auto rr = gw.submit(exit_of("DOTUSDT"));
        std::printf("  ...an automated EXIT is BACKED OFF, not refused forever -> ok=%d (%s)\n",
                    (int)rr.ok, rr.error.c_str());
        OrderIntent human = exit_of("DOTUSDT"); human.operator_initiated = true;
        auto hr = gw.submit(human);
        std::printf("  ...an OPERATOR EXIT on the same symbol -> ok=%d\n", (int)hr.ok);
        CHECK(hr.ok);        // the human close ALWAYS gets out
    }

    std::printf("\n=== GATE F: SHADOW mode is untouched (research record byte-identical) ===\n");
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        gw.rate_guard().max_orders_per_sec_ = 1;
        int ok = 0;
        for (int i = 0; i < 30; ++i) if (gw.submit(entry("BTCUSDT")).ok) ++ok;
        std::printf("  30 SHADOW entries with a 1/s cap -> %d passed (guards are LIVE-only)\n", ok);
        CHECK(ok == 30);      // stated limitation, verified rather than assumed
    }

    std::printf("\n%s (%d failure(s))\n", fails ? "*** TEST FAILED ***" : "ALL STORM-GUARD CHECKS PASSED", fails);
    return fails ? 1 : 0;
}

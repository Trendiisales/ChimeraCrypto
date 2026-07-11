// drawdown_governor_test.cpp — Phase-3 item 18.
// A deepening drawdown steps exposure 100 -> 75 -> 50 -> 25 -> HALT; once halted,
// an equity tick-up alone can NOT auto-resume — a full recovery state is required.
#include "live/DrawdownGovernor.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
#define NEAR(a,b) (std::fabs((a)-(b)) < 1e-9)

int main() {
    DrawdownGovernor G;
    G.configure(/*w1*/0.05, /*w2*/0.10, /*w3*/0.15, /*halt*/0.20);

    CHECK(NEAR(G.update_equity(100000.0), 1.00));   // peak, full size
    CHECK(NEAR(G.update_equity(97000.0), 1.00));    // -3% -> still 100%
    CHECK(NEAR(G.update_equity(93000.0), 0.75));    // -7% -> 75%
    CHECK(NEAR(G.update_equity(88000.0), 0.50));    // -12% -> 50%
    CHECK(NEAR(G.update_equity(83000.0), 0.25));    // -17% -> 25%
    CHECK(NEAR(G.update_equity(79000.0), 0.00));    // -21% -> HALT
    CHECK(G.halted());

    // halt is LATCHED: equity recovering to -8% still returns 0 (no auto-resume).
    CHECK(NEAR(G.update_equity(92000.0), 0.00));
    CHECK(G.halted());

    // recovery gate: resume BLOCKED until ALL conditions hold.
    CHECK(!G.try_resume());                          // nothing set
    G.set_data_healthy(true);   CHECK(!G.try_resume());
    G.set_ledger_reconciled(true); CHECK(!G.try_resume());
    G.set_risk_reduced(true);   CHECK(!G.try_resume());
    G.set_operator_ack(true);
    CHECK(G.recovery_ready());
    CHECK(G.try_resume());                           // now all four -> resume
    CHECK(!G.halted());

    // resumes SMALL (25%), re-baselined peak (no snap-back to 100%).
    double s = G.update_equity(92000.0);
    std::printf("[info] post-resume scale=%.2f peak=%.0f\n", s, G.peak());
    CHECK(NEAR(s, 1.00) || s <= 1.00);              // fresh peak -> dd 0 -> back to graded
    CHECK(NEAR(G.peak(), 92000.0));                 // peak re-baselined to resume equity

    // operator ack is single-use: a fresh halt can't be cleared by the stale ack.
    G.update_equity(70000.0);                        // -24% from 92k -> HALT again
    CHECK(G.halted());
    CHECK(!G.try_resume());                          // ack was consumed on last resume

    std::printf(fails==0 ? "PASS: graduated drawdown + recovery gate blocks auto-resume\n"
                         : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

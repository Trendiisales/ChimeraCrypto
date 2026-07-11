// Phase-4 item 21 — gate attribution + counterfactual + correlation-ID.
// Tests: (a) a suppressed signal produces an attribution record carrying
// per-gate reasons AND a RESOLVABLE correlation-ID; (b) the counterfactual
// resolves forward from prices and per-gate stats separate a HELPFUL gate
// (dodged a loser) from a SUSPECT gate (killed a winner); (c) a passed signal
// is recorded as entered.
#include "live/GateAttribution.hpp"
#include <cstdio>

using namespace chimera;

int main() {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) { std::printf("  FAIL: %s\n", msg); failures++; }
    };

    GateAttribution attr;
    attr.configure(/*horizon_ms*/ 3600 * 1000, /*tp_bp*/ 0, /*sl_bp*/ 0);

    // (a) a suppressed signal -> record with reason + resolvable corr-id
    uint64_t cid = attr.begin_signal("BTC-TSMOM-D1", "btcusdt", "TSMOM",
                                     /*px*/ 100.0, /*ts*/ 0);
    attr.suppressed(cid, "VOL_REGIME", "HIGH vol — trend-following SUPPRESSED");
    const GateRecord* rec = attr.find(cid);
    check(rec != nullptr, "correlation-ID is resolvable");
    check(rec && rec->suppressed, "record marked suppressed");
    check(rec && rec->first_gate == "VOL_REGIME", "first gate captured");
    check(rec && rec->hits.size() == 1 && rec->hits[0].reason.find("HIGH vol") != std::string::npos,
          "per-gate reason recorded");
    std::printf("  (a) corr_id=%llu first_gate=%s reason='%s'\n",
                (unsigned long long)rec->corr_id, rec->first_gate.c_str(),
                rec->hits[0].reason.c_str());

    // (b1) counterfactual — this suppressed trade would have WON (+200bp) ->
    //      VOL_REGIME looks SUSPECT (it killed a winner).
    attr.on_price("btcusdt", 102.0, /*ts*/ 3600 * 1000 + 1);   // +200bp after horizon
    rec = attr.find(cid);
    check(rec && rec->cf_resolved, "counterfactual resolved");
    check(rec && std::fabs(rec->cf_return_bp - 200.0) < 1e-6, "counterfactual return = +200bp");
    std::printf("  (b1) cf_return=%+.0fbp reason=%s\n", rec->cf_return_bp, rec->cf_reason);

    // (b2) a second suppressed signal on a DIFFERENT gate that would have LOST
    //      -> that gate is HELPFUL (dodged a loser).
    uint64_t cid2 = attr.begin_signal("SOL-RSI-H1", "solusdt", "RSI_REVERT", 50.0, 0);
    attr.suppressed(cid2, "SESSION_FILTER", "Asian session — SUPPRESSED");
    attr.on_price("solusdt", 49.0, 3600 * 1000 + 1);           // -200bp
    auto stats = attr.per_gate_stats();
    check(stats.count("VOL_REGIME") && stats.count("SESSION_FILTER"), "both gates have stats");
    check(stats["VOL_REGIME"].avg_bp() > 0.0 && !stats["VOL_REGIME"].helpful(),
          "VOL_REGIME suppressed a winner -> SUSPECT");
    check(stats["SESSION_FILTER"].avg_bp() < 0.0 && stats["SESSION_FILTER"].helpful(),
          "SESSION_FILTER suppressed a loser -> HELPFUL");
    std::printf("  (b2) VOL_REGIME avg=%+.0fbp(%s)  SESSION_FILTER avg=%+.0fbp(%s)\n",
                stats["VOL_REGIME"].avg_bp(), stats["VOL_REGIME"].helpful()?"help":"suspect",
                stats["SESSION_FILTER"].avg_bp(), stats["SESSION_FILTER"].helpful()?"help":"suspect");

    // (c) a passed signal is recorded as entered (no counterfactual needed)
    uint64_t cid3 = attr.begin_signal("ETH-DON-D1", "ethusdt", "DONCHIAN", 2000.0, 0);
    attr.passed(cid3);
    rec = attr.find(cid3);
    check(rec && rec->entered && !rec->suppressed, "passed signal recorded as entered");

    if (failures == 0) { std::printf("PASS gate_attribution_test\n"); return 0; }
    std::printf("FAIL gate_attribution_test (%d)\n", failures); return 1;
}

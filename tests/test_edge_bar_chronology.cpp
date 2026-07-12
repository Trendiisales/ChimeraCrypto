// ─────────────────────────────────────────────────────────────────────────────
// test_edge_bar_chronology.cpp  (S-2026-07-13, audit CH-01 regression test)
//
// Proves the EdgeEngine bar builder never emits a bar whose open_ts_ms is <= the
// previous emitted bar — i.e. a late / out-of-order tick cannot rewind bar
// chronology and re-close an already-closed period.
//
// The audit reproduced the defect with tick sequence 120000,180000,120000,180000
// on a 60s engine: the second 120000 tick rewound cur_bar_id_ and re-closed the
// 120000 bar, so the emitted open_ts_ms stream went ...,120000,180000,120000,...
// (non-monotonic → corrupted indicator history).
//
// Build (standalone, no libwebsockets / no live deps):
//   g++ -O2 -std=c++20 -I../include tests/test_edge_bar_chronology.cpp -o /tmp/edge_chrono_test
// Run:
//   /tmp/edge_chrono_test    # exit 0 = all pass, non-zero = a case failed
// ─────────────────────────────────────────────────────────────────────────────
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <vector>
#include <cstdlib>

using chimera::EdgeEngine;

// Capture every emitted bar's open_ts_ms via the on_bar callback.
struct Capture {
    std::vector<int64_t> emitted_ts;
    void hook(EdgeEngine& e) {
        e.set_on_bar([this](const EdgeEngine::BarRecord& b) {
            emitted_ts.push_back(b.open_ts_ms);
        });
    }
    // strictly-increasing check — the property CH-01 violates.
    bool strictly_monotonic() const {
        for (size_t i = 1; i < emitted_ts.size(); ++i)
            if (emitted_ts[i] <= emitted_ts[i - 1]) return false;
        return true;
    }
};

static EdgeEngine::Config cfg60(const char* tag) {
    EdgeEngine::Config c;
    c.symbol = "testusdt"; c.tag = tag;
    c.tf_secs = 60;                 // 60-second bars → bar_id = ts_ms/1000/60
    c.lookback = 5; c.atr_period = 3; c.max_history = 500;
    return c;
}

static int failures = 0;
static void check(bool ok, const char* name, const Capture& cap) {
    std::printf("[%s] %s  (emitted %zu bars)\n", ok ? "PASS" : "FAIL", name, cap.emitted_ts.size());
    if (!ok) {
        ++failures;
        std::printf("        emitted open_ts_ms:");
        for (auto t : cap.emitted_ts) std::printf(" %lld", (long long)t);
        std::printf("\n");
    }
}

int main() {
    // ── Case 1: the exact audit reproducer — late tick must NOT rewind ──────
    {
        EdgeEngine e(cfg60("CH01-repro")); Capture cap; cap.hook(e);
        // 60s bars: 120000ms→bar2000, 180000ms→bar3000... use round minute ms.
        // Sequence: t=120000 (bar A), 180000 (bar B, closes A), 120000 (LATE),
        //           180000 (LATE). The two late ticks must be dropped; only ONE
        //           bar (A) should ever close, at open_ts_ms=120000.
        e.on_tick(100.0, 120000);
        e.on_tick(101.0, 180000);   // closes bar A (open 120000)
        e.on_tick( 99.0, 120000);   // LATE — must be ignored
        e.on_tick(102.0, 180000);   // LATE — must be ignored
        check(cap.strictly_monotonic() && cap.emitted_ts.size() == 1,
              "audit reproducer: late ticks dropped, one clean close", cap);
    }

    // ── Case 2: normal monotonic stream still builds every bar (no regression) ─
    {
        EdgeEngine e(cfg60("mono")); Capture cap; cap.hook(e);
        for (int i = 0; i < 10; ++i) e.on_tick(100.0 + i, (int64_t)(60000LL * (i + 2)));
        // 10 ticks each in its own 60s bar → 9 closes, strictly increasing.
        check(cap.strictly_monotonic() && cap.emitted_ts.size() == 9,
              "monotonic stream builds every bar (regression guard)", cap);
    }

    // ── Case 3: real gap-fill still works (feed silence → filler bars) ────────
    {
        EdgeEngine e(cfg60("gap")); Capture cap; cap.hook(e);
        e.on_tick(100.0, 120000);            // bar 2000
        e.on_tick(105.0, 120000 + 5*60000);  // jump 5 bars forward → 4 filler + close
        // closes: bar2000 + 4 synthetic filler bars = 5 emitted, strictly increasing.
        check(cap.strictly_monotonic() && cap.emitted_ts.size() == 5,
              "forward gap still fills (no over/under-fill)", cap);
    }

    // ── Case 4: shuffled/dup burst — every emitted ts strictly increasing ────
    {
        EdgeEngine e(cfg60("shuffle")); Capture cap; cap.hook(e);
        int64_t seq[] = {120000,180000,180000,120000,240000,180000,240000,300000,120000,300000};
        for (int64_t t : seq) e.on_tick(100.0, t);
        check(cap.strictly_monotonic(),
              "shuffled+duplicated burst never rewinds chronology", cap);
    }

    // ── Case 5 (CH-09): zero timeframe is clamped, not a divide-by-zero crash ─
    {
        EdgeEngine::Config c = cfg60("badtf"); c.tf_secs = 0;
        EdgeEngine e(c); Capture cap; cap.hook(e);
        e.on_tick(100.0, 60000); e.on_tick(101.0, 120000);  // would SIGFPE if unclamped
        bool ok = cap.emitted_ts.empty() || cap.strictly_monotonic();
        std::printf("[%s] CH-09 zero-tf clamped, no divide-by-zero\n", ok ? "PASS" : "FAIL");
        if (!ok) ++failures;
    }

    // ── Case 6 (CH-10): malformed seed bars are rejected ─────────────────────
    {
        EdgeEngine e(cfg60("badseed"));
        std::vector<EdgeEngine::SeedBar> bad;
        double nan = std::nan("");
        bad.push_back({60000, 100, 101, 99, 100});      // good
        bad.push_back({120000, nan, 101, 99, 100});     // NaN open -> reject
        bad.push_back({60000, 100, 101, 99, 100});      // non-monotonic ts -> reject
        bad.push_back({180000, 100, 90, 99, 100});      // high<low -> reject
        bad.push_back({240000, 100, 101, 99, 100});     // good
        int inserted = e.seed_bars(bad);
        bool ok = (inserted == 2);   // only the 2 clean bars
        std::printf("[%s] CH-10 seed rejects NaN/non-monotonic/malformed (inserted %d, want 2)\n", ok ? "PASS" : "FAIL", inserted);
        if (!ok) ++failures;
    }

    // ── Case 7 (CH-10): corrupt resume state is rejected ─────────────────────
    {
        EdgeEngine e(cfg60("badresume"));
        EdgeEngine::ResumeState rs;
        rs.entry_px = 100.0; rs.sl_px = 500.0;   // stop 5x entry -> implausible
        bool accepted = e.resume_position(rs);
        bool ok = !accepted && !e.in_position();
        std::printf("[%s] CH-10 resume rejects implausible stop (5x entry)\n", ok ? "PASS" : "FAIL");
        if (!ok) ++failures;

        EdgeEngine e2(cfg60("goodresume"));
        EdgeEngine::ResumeState g;
        g.entry_px = 100.0; g.sl_px = 97.0; g.atr_at_entry = 1.0;
        bool accepted2 = e2.resume_position(g);
        bool ok2 = accepted2 && e2.in_position();
        std::printf("[%s] CH-10 resume ACCEPTS a valid state (no false reject)\n", ok2 ? "PASS" : "FAIL");
        if (!ok2) ++failures;
    }

    std::printf("\n%s — %d failure(s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}

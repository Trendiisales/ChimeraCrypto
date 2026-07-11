// Phase-4 item 23 — data-quality gate / stale-data refusal.
// Tests the corruption battery: (a) a clean daily series passes; (b) duplicate
// bars rejected; (c) missing days (gap) rejected; (d) out-of-order timestamps
// rejected; (e) invalid prices rejected; (f) STALE history refused. Each with
// a clear reason. Also checksum stability + schema mismatch.
#include "live/DataQuality.hpp"
#include <cstdio>
#include <vector>

using namespace chimera;

static const int64_t DAY = 86400LL * 1000;

static std::vector<DQBar> clean_series(int n, int64_t t0 = 0) {
    std::vector<DQBar> v;
    for (int i = 0; i < n; ++i) {
        DQBar b; b.ts_ms = t0 + (int64_t)i * DAY;
        double px = 100.0 + i;
        b.o = px; b.h = px + 2; b.l = px - 2; b.c = px + 1;
        v.push_back(b);
    }
    return v;
}

int main() {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) { std::printf("  FAIL: %s\n", msg); failures++; }
    };

    DataQualityGate g;
    g.configure(/*step*/ DAY, /*max_stale*/ 3 * DAY, /*schema*/ 1);
    int64_t now = 30 * DAY;   // "now" is day 30

    // (a) clean series passes
    {
        auto v = clean_series(30, 0);      // days 0..29, last is 1 day old -> fresh
        auto r = g.validate(v, now);
        check(r.ok, "clean series passes");
        check(r.duplicates == 0 && r.gaps == 0 && r.out_of_order == 0 && r.invalid == 0,
              "no anomalies flagged on clean data");
        std::printf("  (a) clean: ok=%d rows=%d checksum=%llu\n",
                    r.ok, r.rows, (unsigned long long)r.checksum);
    }
    // (b) duplicate bar rejected
    {
        auto v = clean_series(10, 0);
        v.push_back(v.back());              // duplicate last ts
        auto r = g.validate(v, now);
        check(!r.ok && r.duplicates > 0, "duplicate bar rejected");
        check(r.reason.find("duplicate") != std::string::npos, "reason names duplicate");
        std::printf("  (b) dup: %s\n", r.reason.c_str());
    }
    // (c) missing day (gap) rejected
    {
        auto v = clean_series(10, 0);
        v.erase(v.begin() + 5);            // remove day 5 -> gap between 4 and 6
        auto r = g.validate(v, now);
        check(!r.ok && r.gaps > 0, "gap rejected");
        check(r.reason.find("missing") != std::string::npos, "reason names missing bars");
        std::printf("  (c) gap: %s\n", r.reason.c_str());
    }
    // (d) out-of-order rejected
    {
        auto v = clean_series(10, 0);
        std::swap(v[3].ts_ms, v[6].ts_ms); // scramble timestamps
        auto r = g.validate(v, now);
        check(!r.ok && (r.out_of_order > 0 || r.gaps > 0), "out-of-order rejected");
        std::printf("  (d) ooo: %s\n", r.reason.c_str());
    }
    // (e) invalid price rejected
    {
        auto v = clean_series(10, 0);
        v[4].c = -1.0;                     // negative close
        auto r = g.validate(v, now);
        check(!r.ok && r.invalid > 0, "invalid price rejected");
        check(r.reason.find("invalid price") != std::string::npos, "reason names invalid price");
        std::printf("  (e) invalid: %s\n", r.reason.c_str());
        // high<low
        auto v2 = clean_series(10, 0); v2[2].h = v2[2].l - 5.0;
        auto r2 = g.validate(v2, now);
        check(!r2.ok && r2.reason.find("high<low") != std::string::npos, "high<low rejected");
    }
    // (f) STALE history refused (last bar 10 days old vs max 3)
    {
        auto v = clean_series(10, 0);      // last ts = day 9; now=day 30 -> 21d old
        auto r = g.validate(v, now);
        check(!r.ok, "stale series refused");
        check(r.reason.find("STALE") != std::string::npos, "reason says STALE");
        std::printf("  (f) stale: %s\n", r.reason.c_str());
    }
    // checksum determinism + sensitivity
    {
        auto a = clean_series(20, 0), b = clean_series(20, 0), c = clean_series(20, 0);
        c[7].c += 0.01;
        check(DataQualityGate::checksum_of(a) == DataQualityGate::checksum_of(b),
              "checksum deterministic for identical data");
        check(DataQualityGate::checksum_of(a) != DataQualityGate::checksum_of(c),
              "checksum changes when a price changes");
    }

    if (failures == 0) { std::printf("PASS data_quality_test\n"); return 0; }
    std::printf("FAIL data_quality_test (%d)\n", failures); return 1;
}

#pragma once
// data_guard.hpp — every backtest/search MUST load bars through load_guarded().
//
// Why (2026-06-14 disaster): harnesses loaded raw CSVs with no checks, assumed
// multi-year coverage, and bucketed trades into calendar windows that had ZERO
// bars -- scoring garbage as if it were real. This guard makes that impossible:
//   - aborts on out-of-order / duplicate timestamps, non-positive or OHLC-invalid bars
//   - detects the modal interval and aborts if it disagrees with what the harness expects
//   - coverage(bars, a, b) returns bars-in-window; harness MUST assert > min before scoring
// Pair with tools/validate_dataset.py (front gate + MANIFEST). Belt and braces.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace guard {

struct Bar { int64_t ts; double o,h,l,c; };
struct Series {
    std::vector<Bar> bars;
    int64_t interval_ms = 0;
    int64_t first_ts = 0, last_ts = 0;
    std::string sym;
};

// Load + validate. Aborts the process on any integrity violation (fail loud,
// never silently produce a bad backtest). expect_interval_ms<=0 = auto-detect.
inline Series load_guarded(const std::string& path, const std::string& sym,
                           int64_t expect_interval_ms = 0) {
    Series s; s.sym = sym;
    std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "[GUARD] FATAL cannot open %s\n", path.c_str()); std::abort(); }
    std::string ln; std::getline(f, ln); // header
    while (std::getline(f, ln)) {
        std::stringstream ss(ln); std::string a; Bar b{}; int i=0; bool ok=true;
        while (std::getline(ss, a, ',')) {
            try {
                if (i==0) b.ts=std::stoll(a); else if (i==1) b.o=std::stod(a);
                else if (i==2) b.h=std::stod(a); else if (i==3) b.l=std::stod(a);
                else if (i==4) { b.c=std::stod(a); break; }
            } catch (...) { ok=false; break; }
            i++;
        }
        if (ok && i>=4) s.bars.push_back(b);
    }
    if (s.bars.size() < 100) {
        std::fprintf(stderr, "[GUARD] FATAL %s: only %zu bars\n", sym.c_str(), s.bars.size());
        std::abort();
    }
    // integrity scan
    std::vector<int64_t> deltas;
    int nonmono=0, dupes=0, ohlc_bad=0, nonpos=0;
    for (size_t i=0;i<s.bars.size();++i) {
        const Bar& b=s.bars[i];
        if (b.o<=0||b.h<=0||b.l<=0||b.c<=0) nonpos++;
        if (b.h < std::max(b.o,b.c)-1e-9 || b.l > std::min(b.o,b.c)+1e-9 || b.h < b.l-1e-9) ohlc_bad++;
        if (i>0) { int64_t d=b.ts-s.bars[i-1].ts;
            if (d==0) dupes++; else if (d<0) nonmono++; else deltas.push_back(d); }
    }
    if (nonmono || dupes || nonpos || ohlc_bad > (int)s.bars.size()/1000) {
        std::fprintf(stderr, "[GUARD] FATAL %s integrity: nonmono=%d dupes=%d nonpos=%d ohlc_bad=%d\n",
            sym.c_str(), nonmono, dupes, nonpos, ohlc_bad);
        std::abort();
    }
    std::sort(deltas.begin(), deltas.end());
    s.interval_ms = deltas[deltas.size()/2]; // median
    s.first_ts = s.bars.front().ts; s.last_ts = s.bars.back().ts;
    if (expect_interval_ms > 0 && s.interval_ms != expect_interval_ms) {
        std::fprintf(stderr, "[GUARD] FATAL %s interval %lldms != expected %lldms (wrong TF file?)\n",
            sym.c_str(), (long long)s.interval_ms, (long long)expect_interval_ms);
        std::abort();
    }
    return s;
}

// Bars whose ts falls in [a,b). Harness asserts this is >= a minimum before
// scoring a window -- so a window with no data can never be silently counted.
inline int coverage(const Series& s, int64_t a, int64_t b) {
    int n=0; for (const auto& bar : s.bars) if (bar.ts>=a && bar.ts<b) n++; return n;
}

// Abort if any required window is under-covered. Call once per dataset before scoring.
inline void assert_windows(const Series& s, const std::vector<std::pair<int64_t,int64_t>>& wins,
                           int min_bars, const std::vector<std::string>& names) {
    for (size_t i=0;i<wins.size();++i) {
        int n = coverage(s, wins[i].first, wins[i].second);
        if (n < min_bars) {
            std::fprintf(stderr, "[GUARD] FATAL %s window '%s' has %d bars (< %d) -- data does not "
                "cover this window; refusing to score it\n",
                s.sym.c_str(), names[i].c_str(), n, min_bars);
            std::abort();
        }
    }
}

} // namespace guard

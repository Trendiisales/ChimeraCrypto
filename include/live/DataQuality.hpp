#pragma once
// ============================================================================
// DataQuality — Phase-4 review fix, item 23 (DATA QUALITY / STALE REFUSAL).
//
// Warm-start / seed / feed files (data/xsec_seed/*.csv, data/bars/*.ndjson,
// klines) load today with NO schema, checksum, date-range, duplicate, gap, or
// staleness validation — a corrupted or stale history silently seeds the
// engines. This gate validates a parsed bar series and REFUSES malformed or
// stale data with a clear reason, so a bad seed is rejected rather than traded.
//
// Checks: schema version, row count, invalid prices (<=0, h<l, o/c outside
// [l,h]), duplicate timestamps, out-of-order timestamps, gaps vs the expected
// bar step, and STALE history (last bar older than a max-age threshold).
//
// Header-only, dependency-free, unit-tested standalone.
// ============================================================================
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace chimera {

struct DQBar {
    int64_t ts_ms = 0;
    double  o = 0.0, h = 0.0, l = 0.0, c = 0.0, v = 0.0;
};

struct DataQualityReport {
    bool        ok = false;
    std::string reason;      // populated on failure (clear, actionable)
    int         rows = 0;
    int64_t     first_ts = 0, last_ts = 0;
    uint64_t    checksum = 0;
    int         duplicates = 0, gaps = 0, out_of_order = 0, invalid = 0;
    int64_t     age_ms = 0;  // now - last_ts (0 if now not supplied)
};

class DataQualityGate {
public:
    // expected_step_ms: nominal bar spacing (0 => skip gap/ordering-by-step,
    //   still checks dup + monotonic). max_stale_ms: reject if the last bar is
    //   older than this vs `now_ms` (0 => skip staleness). schema_version: the
    //   version this loader understands (mismatch on an explicit file => reject).
    void configure(int64_t expected_step_ms, int64_t max_stale_ms, int schema_version = 1) {
        step_ms_ = expected_step_ms; max_stale_ms_ = max_stale_ms; schema_ = schema_version;
    }
    // How much a gap must exceed the step (in units of steps) before it counts.
    void set_gap_tolerance(double steps) { gap_tol_ = steps; }
    // Reject on ANY gap (default true — a missing day is a hard failure).
    void set_reject_on_gap(bool b) { reject_gap_ = b; }
    // Fractional tolerance for the o/c-within-[l,h] sanity (default tiny).
    void set_price_tolerance(double frac) { price_tol_ = frac; }

    static uint64_t checksum_of(const std::vector<DQBar>& bars) {
        // FNV-1a over the packed bit patterns of (ts,o,h,l,c).
        uint64_t h = 1469598103934665603ULL;
        auto mix = [&](uint64_t x){ h ^= x; h *= 1099511628257ULL; };
        auto bits = [](double d){ uint64_t u; __builtin_memcpy(&u, &d, 8); return u; };
        for (const auto& b : bars) {
            mix((uint64_t)b.ts_ms); mix(bits(b.o)); mix(bits(b.h));
            mix(bits(b.l)); mix(bits(b.c));
        }
        return h;
    }

    DataQualityReport validate(const std::vector<DQBar>& bars, int64_t now_ms = 0) const {
        DataQualityReport r;
        r.rows = (int)bars.size();
        if (bars.empty()) { r.reason = "empty series (0 rows)"; return r; }
        r.checksum = checksum_of(bars);
        r.first_ts = bars.front().ts_ms;
        r.last_ts  = bars.back().ts_ms;

        int64_t prev_ts = 0; bool have_prev = false;
        for (size_t i = 0; i < bars.size(); ++i) {
            const DQBar& b = bars[i];
            // invalid prices
            if (b.o <= 0.0 || b.h <= 0.0 || b.l <= 0.0 || b.c <= 0.0) {
                r.invalid++;
                if (r.reason.empty())
                    r.reason = "invalid price (<=0) at row " + std::to_string(i)
                             + " ts=" + std::to_string(b.ts_ms);
            } else if (b.h < b.l) {
                r.invalid++;
                if (r.reason.empty())
                    r.reason = "high<low at row " + std::to_string(i)
                             + " ts=" + std::to_string(b.ts_ms);
            } else {
                double lo = b.l * (1.0 - price_tol_), hi = b.h * (1.0 + price_tol_);
                if (b.o < lo || b.o > hi || b.c < lo || b.c > hi) {
                    r.invalid++;
                    if (r.reason.empty())
                        r.reason = "open/close outside [low,high] at row " + std::to_string(i)
                                 + " ts=" + std::to_string(b.ts_ms);
                }
            }
            // ordering + duplicates + gaps
            if (have_prev) {
                if (b.ts_ms == prev_ts) {
                    r.duplicates++;
                    if (r.reason.empty())
                        r.reason = "duplicate timestamp " + std::to_string(b.ts_ms)
                                 + " at row " + std::to_string(i);
                } else if (b.ts_ms < prev_ts) {
                    r.out_of_order++;
                    if (r.reason.empty())
                        r.reason = "out-of-order timestamp " + std::to_string(b.ts_ms)
                                 + " < " + std::to_string(prev_ts) + " at row " + std::to_string(i);
                } else if (step_ms_ > 0) {
                    int64_t delta = b.ts_ms - prev_ts;
                    if ((double)delta > (double)step_ms_ * (1.0 + gap_tol_)) {
                        r.gaps++;
                        if (r.reason.empty() && reject_gap_) {
                            int64_t missing = delta / step_ms_ - 1;
                            r.reason = "gap: " + std::to_string(missing)
                                     + " missing bar(s) before ts=" + std::to_string(b.ts_ms)
                                     + " (delta=" + std::to_string(delta)
                                     + "ms step=" + std::to_string(step_ms_) + "ms)";
                        }
                    }
                }
            }
            prev_ts = b.ts_ms; have_prev = true;
        }

        // staleness
        if (now_ms > 0) {
            r.age_ms = now_ms - r.last_ts;
            if (max_stale_ms_ > 0 && r.age_ms > max_stale_ms_ && r.reason.empty()) {
                r.reason = "STALE history: last bar is " + std::to_string(r.age_ms / 1000)
                         + "s old (max " + std::to_string(max_stale_ms_ / 1000) + "s)";
            }
        }

        bool hard_fail = r.invalid > 0 || r.duplicates > 0 || r.out_of_order > 0
                       || (reject_gap_ && r.gaps > 0)
                       || (now_ms > 0 && max_stale_ms_ > 0 && r.age_ms > max_stale_ms_);
        r.ok = !hard_fail;
        if (r.ok) r.reason.clear();
        return r;
    }

    // Parse + validate a simple "ts,o,h,l,c[,v]" CSV (header auto-skipped). A
    // schema-version comment line "# schema=N" at the top is honoured; a
    // mismatch is a hard failure. Missing file => ok=false ("file not found").
    DataQualityReport validate_csv(const std::string& path, int64_t now_ms = 0) const {
        DataQualityReport r;
        std::ifstream f(path);
        if (!f) { r.reason = "file not found: " + path; return r; }
        std::vector<DQBar> bars;
        std::string line; bool first = true;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            if (line[0] == '#') {
                auto p = line.find("schema=");
                if (p != std::string::npos) {
                    int v = std::atoi(line.c_str() + p + 7);
                    if (v != schema_) { r.reason = "schema mismatch: file="
                        + std::to_string(v) + " loader=" + std::to_string(schema_); return r; }
                }
                continue;
            }
            // skip a non-numeric header row
            if (first && !line.empty() && !(std::isdigit((unsigned char)line[0]) || line[0]=='-')) {
                first = false; continue;
            }
            first = false;
            DQBar b; const char* p = line.c_str(); char* e;
            b.ts_ms = std::strtoll(p, &e, 10); if (e == p) continue;
            auto nextd = [&](char*& s)->double{ while (*s==','||*s==' ') ++s; char* ee; double d=std::strtod(s,&ee); s=ee; return d; };
            char* s = e;
            b.o = nextd(s); b.h = nextd(s); b.l = nextd(s); b.c = nextd(s);
            bars.push_back(b);
        }
        return validate(bars, now_ms);
    }

private:
    int64_t step_ms_ = 0;
    int64_t max_stale_ms_ = 0;
    int     schema_ = 1;
    double  gap_tol_ = 0.5;      // a gap must exceed 1.5x step to count
    bool    reject_gap_ = true;
    double  price_tol_ = 1e-6;
};

} // namespace chimera

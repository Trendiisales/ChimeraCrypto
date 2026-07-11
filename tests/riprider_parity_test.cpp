// ============================================================================
// riprider_parity_test.cpp — Phase-1 regression test for the RipRider live-entry fix.
//
// Proves the confirmed live/backtest divergence is CLOSED:
//   (1) the historical BATCH replay (seed_daily_bar + simulate()) and
//   (2) the streaming DAY-ROLL path (on_tick with per-day open/close ticks)
// produce IDENTICAL entries; and streaming now takes > 0 entries (the old code
// could never fire a live entry because evaluate_day(size-1) had no i+1 bar).
//
// Build: g++ -std=c++20 -O2 -I../include riprider_parity_test.cpp -o riprider_parity_test
// (run via tests/run_phase1_tests.sh). Exit 0 = PASS, non-zero = FAIL.
// ============================================================================
#include "core/RipRiderEngine.hpp"
#include <cstdio>
#include <vector>
#include <string>
#include <cmath>

using namespace chimera;

struct Entry { std::string sym; double px; int64_t day; };

// Synthetic universe: a monotonically-rising BTC (always bull, so the regime
// gate passes throughout) and one alt "SOL" with two distinct rips.
struct Bar { int64_t day; double btc_o, btc_c, sol_o, sol_c; };

static std::vector<Bar> make_data() {
    std::vector<Bar> d;
    double btc = 20000.0, sol = 100.0;
    for (int i = 0; i < 340; ++i) {
        double btc_o = btc, sol_o = sol;
        btc *= 1.002;                       // BTC drifts up ~0.2%/day -> always bull
        // Rip #1: days 60..66 SOL jumps ~+12%/day (5d return >> 20% -> ignition)
        if (i >= 60 && i <= 66) sol *= 1.12;
        // Rip #2: days 200..206 another jump (tests re-entry after the first exit)
        else if (i >= 200 && i <= 206) sol *= 1.15;
        else sol *= 1.0;                    // flat otherwise
        d.push_back({ (int64_t)i, btc_o, btc, sol_o, sol });
    }
    return d;
}

static std::vector<Entry> run_batch(const std::vector<Bar>& data) {
    std::vector<Entry> out;
    RipRiderEngine eng;                     // defaults = validated config
    eng.set_universe({"BTC", "SOL"});
    eng.set_entry_callback([&](const std::string& s, double px, int64_t ts) {
        out.push_back({ s, px, ts / 86400000LL });
    });
    for (const auto& b : data) {
        eng.seed_daily_bar("BTC", b.day, b.btc_o, b.btc_c);
        eng.seed_daily_bar("SOL", b.day, b.sol_o, b.sol_c);
    }
    eng.simulate();
    return out;
}

static std::vector<Entry> run_stream(const std::vector<Bar>& data) {
    std::vector<Entry> out;
    RipRiderEngine eng;
    eng.set_universe({"BTC", "SOL"});
    eng.set_entry_callback([&](const std::string& s, double px, int64_t ts) {
        out.push_back({ s, px, ts / 86400000LL });
    });
    // For each UTC day feed: all opens (first tick of the day), then all closes.
    // The first open tick of the next day triggers the roll-over of the prior day.
    for (const auto& b : data) {
        int64_t day_ms = b.day * 86400000LL;
        eng.on_tick("BTC", b.btc_o, day_ms + 1);
        eng.on_tick("SOL", b.sol_o, day_ms + 2);
        eng.on_tick("BTC", b.btc_c, day_ms + 86399000LL);
        eng.on_tick("SOL", b.sol_c, day_ms + 86399500LL);
    }
    return out;
}

int main() {
    auto data   = make_data();
    auto batch  = run_batch(data);
    auto stream = run_stream(data);

    std::printf("[RIPRIDER-PARITY] batch entries=%zu  stream entries=%zu\n",
                batch.size(), stream.size());
    for (size_t i = 0; i < batch.size(); ++i)
        std::printf("   batch [%zu] %s @ %.4f day=%lld\n", i,
                    batch[i].sym.c_str(), batch[i].px, (long long)batch[i].day);
    for (size_t i = 0; i < stream.size(); ++i)
        std::printf("   stream[%zu] %s @ %.4f day=%lld\n", i,
                    stream[i].sym.c_str(), stream[i].px, (long long)stream[i].day);

    bool pass = true;
    if (stream.empty()) {
        std::printf("FAIL: streaming produced ZERO entries (the live-entry bug is NOT fixed)\n");
        pass = false;
    }
    if (batch.size() != stream.size()) {
        std::printf("FAIL: entry COUNT differs batch=%zu stream=%zu\n", batch.size(), stream.size());
        pass = false;
    } else {
        for (size_t i = 0; i < batch.size(); ++i) {
            bool same = batch[i].sym == stream[i].sym
                     && std::fabs(batch[i].px - stream[i].px) < 1e-9
                     && batch[i].day == stream[i].day;
            if (!same) {
                std::printf("FAIL: entry[%zu] differs: batch %s@%.6f d=%lld vs stream %s@%.6f d=%lld\n",
                            i, batch[i].sym.c_str(), batch[i].px, (long long)batch[i].day,
                            stream[i].sym.c_str(), stream[i].px, (long long)stream[i].day);
                pass = false;
            }
        }
    }

    std::printf(pass ? "PASS: batch == stream entries, and stream entered %zu positions\n"
                     : "FAILED\n", stream.size());
    return pass ? 0 : 1;
}

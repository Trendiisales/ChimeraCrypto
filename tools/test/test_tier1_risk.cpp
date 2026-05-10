// ============================================================================
// test_tier1_risk.cpp
// Header-only test suite for include/risk/Tier1Risk.hpp.
// Compile:
//   g++ -std=c++20 -Wall -Wextra -Werror -O2  -I include  tools/test/test_tier1_risk.cpp  -o /tmp/test_tier1_risk  -lpthread
// Run:
//   /tmp/test_tier1_risk
// ============================================================================

#include "risk/Tier1Risk.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace chimera::risk;

// ── tiny test harness ───────────────────────────────────────────────────────
static int g_pass = 0;
static int g_fail = 0;

#define EXPECT(cond, msg) do { \
    if (cond) { ++g_pass; std::printf("  PASS: %s\n", msg); } \
    else      { ++g_fail; std::printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while (0)

// Inject a deterministic clock so tests don't depend on wall time.
static int64_t g_test_clock_ms = 1'700'000'000'000LL;  // arbitrary fixed start
static int64_t fake_clock() { return g_test_clock_ms; }

static std::string tmp_state_path(const char* tag) {
    return std::string("/tmp/tier1_test_") + tag + "_state.json";
}

static void cleanup(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

// ── Scenario 1: per-engine R cap ────────────────────────────────────────────
static void test_per_engine_r_cap() {
    std::printf("\n=== Scenario 1: per-engine R cap ===\n");
    Tier1Risk::Config cfg;
    cfg.per_engine_r_cap.fill(1.0);
    cfg.state_path = tmp_state_path("s1");
    cleanup(cfg.state_path);

    Tier1Risk risk(cfg);
    risk.set_clock(fake_clock);

    // Engine starts with full R available
    EXPECT(risk.available_R(EngineType::ETH_BTC_LEADLAG, 0) == 1.0,
        "fresh engine has full R available");

    // Take half the budget — should still have half left
    risk.on_position_open(EngineType::ETH_BTC_LEADLAG, 0, true, 0.5);
    EXPECT(std::abs(risk.available_R(EngineType::ETH_BTC_LEADLAG, 0) - 0.5) < 1e-9,
        "after opening 0.5R, 0.5R remains");

    // Take the rest
    risk.on_position_open(EngineType::ETH_BTC_LEADLAG, 0, true, 0.5);
    EXPECT(risk.available_R(EngineType::ETH_BTC_LEADLAG, 0) == 0.0,
        "after exhausting cap, available_R is 0");

    // authorize_entry should refuse with reason per_engine_cap
    auto d = risk.authorize_entry(EngineType::ETH_BTC_LEADLAG, 0, true, 0.5);
    EXPECT(!d.ok && std::strcmp(d.reason, "per_engine_cap") == 0,
        "authorize_entry returns per_engine_cap when at cap");

    // Close the position — should free the budget
    risk.on_position_close(EngineType::ETH_BTC_LEADLAG, /*pnl_bp=*/+50.0);
    EXPECT(risk.available_R(EngineType::ETH_BTC_LEADLAG, 0) == 1.0,
        "after closing, full budget restored");

    cleanup(cfg.state_path);
}

// ── Scenario 2: correlation cap ─────────────────────────────────────────────
static void test_correlation_cap() {
    std::printf("\n=== Scenario 2: correlation cap ===\n");
    Tier1Risk::Config cfg;
    cfg.per_engine_r_cap.fill(1.0);
    cfg.max_engines_per_symbol_side = 2;   // strict: only 2 engines per sym/side
    cfg.state_path = tmp_state_path("s2");
    cleanup(cfg.state_path);

    Tier1Risk risk(cfg);
    risk.set_clock(fake_clock);

    // Engine A goes long BTC
    risk.on_position_open(EngineType::ETH_BTC_LEADLAG, /*BTC=*/0, /*long=*/true, 1.0);

    // Engine B goes long BTC too — second slot
    auto db = risk.authorize_entry(EngineType::COINBASE_PREMIUM_MREV, 0, true, 1.0);
    EXPECT(db.ok, "second engine going long BTC is allowed (slots = 2)");
    risk.on_position_open(EngineType::COINBASE_PREMIUM_MREV, 0, true, 1.0);

    // Engine C also wants long BTC — should be capped
    auto dc = risk.authorize_entry(EngineType::VOL_COMPRESSION_BREAKOUT, 0, true, 1.0);
    EXPECT(!dc.ok && std::strcmp(dc.reason, "correlation_cap") == 0,
        "third engine going long BTC is correlation-capped");

    // Engine C wanting SHORT BTC (different side) is fine
    auto dc2 = risk.authorize_entry(EngineType::VOL_COMPRESSION_BREAKOUT, 0, false, 1.0);
    EXPECT(dc2.ok, "third engine going SHORT BTC is allowed (different side)");

    // Engine C wanting LONG ETH (different symbol) is fine
    auto dc3 = risk.authorize_entry(EngineType::VOL_COMPRESSION_BREAKOUT, 1, true, 1.0);
    EXPECT(dc3.ok, "third engine going LONG ETH is allowed (different symbol)");

    cleanup(cfg.state_path);
}

// ── Scenario 3: daily loss kill ─────────────────────────────────────────────
static void test_daily_loss_kill() {
    std::printf("\n=== Scenario 3: daily loss kill ===\n");
    Tier1Risk::Config cfg;
    cfg.daily_loss_kill_bp = -100.0;
    cfg.state_path = tmp_state_path("s3");
    cleanup(cfg.state_path);

    Tier1Risk risk(cfg);
    risk.set_clock(fake_clock);

    EXPECT(!risk.is_halted(), "fresh risk wrapper is not halted");

    // Take a few trades, accumulate losses
    risk.on_position_open (EngineType::OBI, 0, true, 1.0);
    risk.on_position_close(EngineType::OBI, -40.0);   // running: -40
    EXPECT(!risk.is_halted(), "after -40bp not halted");

    risk.on_position_open (EngineType::OBI, 0, true, 1.0);
    risk.on_position_close(EngineType::OBI, -40.0);   // running: -80
    EXPECT(!risk.is_halted(), "after -80bp not halted");

    risk.on_position_open (EngineType::OBI, 0, true, 1.0);
    risk.on_position_close(EngineType::OBI, -30.0);   // running: -110, trips
    EXPECT(risk.is_halted(), "after -110bp HALTED (threshold -100bp)");

    // Once halted, available_R returns 0 for all engines
    EXPECT(risk.available_R(EngineType::SWING, 0) == 0.0,
        "halted: SWING engine sees 0 available_R");
    EXPECT(risk.available_R(EngineType::ETH_BTC_LEADLAG, 0) == 0.0,
        "halted: ETH_BTC_LEADLAG engine sees 0 available_R");

    // Resume manually
    risk.resume_all();
    EXPECT(!risk.is_halted(), "after resume_all, not halted");
    EXPECT(risk.available_R(EngineType::SWING, 0) > 0.0,
        "after resume, SWING has budget again");

    cleanup(cfg.state_path);
}

// ── Scenario 4: state survives reload from disk ─────────────────────────────
static void test_state_persistence() {
    std::printf("\n=== Scenario 4: state persists across restart ===\n");
    const std::string sp = tmp_state_path("s4");
    cleanup(sp);

    // First instance: incur a daily loss, take a position
    {
        Tier1Risk::Config cfg;
        cfg.daily_loss_kill_bp = -500.0;   // high, won't trip
        cfg.state_path = sp;
        Tier1Risk risk(cfg);
        risk.set_clock(fake_clock);

        risk.on_position_open (EngineType::FUNDING_PERSIST_FADE, 0, true, 0.5);
        risk.on_position_close(EngineType::FUNDING_PERSIST_FADE, -75.0);

        // Open a position that we leave open for the reload
        risk.on_position_open (EngineType::ETH_BTC_LEADLAG, 1, true, 0.7);

        auto s = risk.snapshot();
        EXPECT(std::abs(s.daily_realized_bp - (-75.0)) < 1e-9,
            "before restart: daily realized = -75bp");
        EXPECT(std::abs(s.per_engine_open_R[(int)EngineType::ETH_BTC_LEADLAG] - 0.7) < 1e-9,
            "before restart: ETH_BTC_LEADLAG holds 0.7R");
    }

    // Second instance: load from disk
    {
        Tier1Risk::Config cfg;
        cfg.state_path = sp;
        Tier1Risk risk(cfg);

        auto s = risk.snapshot();
        EXPECT(std::abs(s.daily_realized_bp - (-75.0)) < 1e-9,
            "after restart: daily realized still -75bp");
        EXPECT(std::abs(s.per_engine_open_R[(int)EngineType::ETH_BTC_LEADLAG] - 0.7) < 1e-9,
            "after restart: open position restored");
        EXPECT(s.per_engine_open_R[(int)EngineType::FUNDING_PERSIST_FADE] == 0.0,
            "after restart: closed engine has zero open_R");
    }

    cleanup(sp);
}

// ── Scenario 5: manual halt + total exposure cap ────────────────────────────
static void test_manual_halt_and_total_cap() {
    std::printf("\n=== Scenario 5: manual halt + total exposure cap ===\n");
    Tier1Risk::Config cfg;
    cfg.per_engine_r_cap.fill(1.0);
    cfg.total_r_cap = 2.0;     // strict: only 2.0R total across all engines
    cfg.max_engines_per_symbol_side = 8;   // disable correlation cap for this test
    cfg.state_path = tmp_state_path("s5");
    cleanup(cfg.state_path);

    Tier1Risk risk(cfg);
    risk.set_clock(fake_clock);

    // Two engines fully load up = 2.0R total
    risk.on_position_open(EngineType::ETH_BTC_LEADLAG,         0, true, 1.0);
    risk.on_position_open(EngineType::COINBASE_PREMIUM_MREV,   0, true, 1.0);

    // Third engine wants in — denied by total_R_cap
    auto d = risk.authorize_entry(EngineType::VOL_COMPRESSION_BREAKOUT, 1, true, 1.0);
    EXPECT(!d.ok && std::strcmp(d.reason, "total_R_cap") == 0,
        "third engine denied by total_R_cap");

    // Manual halt — should override even when nothing else is wrong
    risk.resume_all();   // ensure not halted from prior test
    risk.halt_all("operator stop button");
    EXPECT(risk.is_halted(), "manual halt sets halted state");
    EXPECT(risk.halt_reason() == "operator stop button",
        "halt_reason matches what was set");

    auto d2 = risk.authorize_entry(EngineType::SWING, 0, true, 0.1);
    EXPECT(!d2.ok && std::strcmp(d2.reason, "halted") == 0,
        "all entries denied with reason 'halted' when manually halted");

    risk.resume_all();
    EXPECT(!risk.is_halted(), "resume_all clears halt");

    cleanup(cfg.state_path);
}

// ── Scenario 6: snapshot_json structural sanity ─────────────────────────────
static void test_snapshot_json() {
    std::printf("\n=== Scenario 6: snapshot_json contains expected keys ===\n");
    Tier1Risk::Config cfg;
    cfg.state_path = tmp_state_path("s6");
    cleanup(cfg.state_path);

    Tier1Risk risk(cfg);
    risk.set_clock(fake_clock);
    risk.on_position_open(EngineType::SWING, 0, true, 0.5);

    const std::string j = risk.snapshot_json();
    EXPECT(j.find("\"halted\":false")            != std::string::npos,
        "json contains halted:false");
    EXPECT(j.find("\"daily_realized_bp\":")      != std::string::npos,
        "json contains daily_realized_bp");
    EXPECT(j.find("\"total_open_R\":")           != std::string::npos,
        "json contains total_open_R");
    EXPECT(j.find("\"SWING\":0.5")               != std::string::npos,
        "json contains SWING engine open R = 0.5");
    EXPECT(j.find("\"ETH_BTC_LEADLAG\":0")       != std::string::npos,
        "json contains ETH_BTC_LEADLAG engine open R = 0");

    cleanup(cfg.state_path);
}

int main() {
    std::printf("Tier1Risk test suite\n");
    std::printf("====================\n");

    test_per_engine_r_cap();
    test_correlation_cap();
    test_daily_loss_kill();
    test_state_persistence();
    test_manual_halt_and_total_cap();
    test_snapshot_json();

    std::printf("\n====================\n");
    std::printf("Result: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}

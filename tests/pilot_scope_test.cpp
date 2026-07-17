// pilot_scope_test.cpp — LIVE PILOT SCOPE regression test (2026-07-18).
// Proves the gateway's go-live pilot bounds:
//   * SHADOW mode: pilot config is INERT — non-pilot symbols, oversize orders
//     all pass exactly as before (shadow research record preserved).
//   * LIVE mode: entry on a non-allowlisted symbol REJECTED; allowlisted passes.
//   * LIVE mode: per-order notional clamped to pilot_max_order_usd.
//   * LIVE mode: aggregate gross cap rejects an entry that would breach it;
//     a SELL reduces tracked gross and re-opens headroom.
//   * Exits are NEVER blocked — even for a non-pilot symbol in LIVE mode.
// Build (same pattern as execution_gateway_test):
//   g++ -std=c++20 -I../include <brew curl/openssl includes> pilot_scope_test.cpp
#include "live/ExecutionGateway.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;

struct MockExec {
    int calls = 0;
    std::string last_sym; bool last_buy = false; double last_qty = 0, last_px = 0;
    OrderResult execute(const std::string& sym, bool is_buy, double qty, double px,
                        const std::string& = "") {
        ++calls; last_sym = sym; last_buy = is_buy; last_qty = qty; last_px = px;
        OrderResult r; r.ok = true; r.status = "FILLED";
        r.executed_qty = qty; r.avg_price = px; return r;
    }
};

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

static void arm_pilot(ExecutionGatewayT<MockExec>& gw) {
    gw.pilot_enabled       = true;
    gw.pilot_symbols       = { "BTCUSDT", "ETHUSDT" };
    gw.pilot_max_order_usd = 12.0;
    gw.pilot_max_gross_usd = 50.0;
}

int main() {
    // 1. SHADOW mode: pilot config INERT — non-pilot symbol + oversize both pass.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        arm_pilot(gw);
        auto r1 = gw.submit({ "DOGEUSDT", true, 100.0, 1.0, false, "T" }); // non-pilot sym
        CHECK(r1.ok && ex.calls == 1);
        auto r2 = gw.submit({ "BTCUSDT", true, 1.0, 50000.0, false, "T" }); // $50k >> cap
        CHECK(r2.ok && ex.calls == 2 && std::fabs(ex.last_qty - 1.0) < 1e-12); // NOT clamped
        CHECK(gw.pilot_gross_usd == 0.0); // shadow never accrues live gross
    }
    // 2. LIVE mode: non-allowlisted symbol entry REJECTED; allowlisted passes.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        arm_pilot(gw);
        auto r1 = gw.submit({ "DOGEUSDT", true, 100.0, 1.0, false, "T" });
        CHECK(!r1.ok && ex.calls == 0);
        auto r2 = gw.submit({ "btcusdt", true, 0.0002, 50000.0, false, "T" }); // $10, lowercase ok
        CHECK(r2.ok && ex.calls == 1);
    }
    // 3. LIVE mode: per-order notional clamp to $12.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        arm_pilot(gw);
        auto r = gw.submit({ "ETHUSDT", true, 10.0, 100.0, false, "T" }); // $1000 asked
        CHECK(r.ok && ex.calls == 1);
        CHECK(std::fabs(ex.last_qty * 100.0 - 12.0) < 1e-9); // clamped to $12
    }
    // 4. LIVE mode: aggregate gross cap — 4x $12 fills, 5th entry rejected;
    //    a SELL reduces gross and re-opens headroom.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        arm_pilot(gw);
        for (int i = 0; i < 4; i++) {
            auto r = gw.submit({ "BTCUSDT", true, 0.00024, 50000.0, false, "T" }); // $12
            CHECK(r.ok);
        }
        CHECK(std::fabs(gw.pilot_gross_usd - 48.0) < 1e-6);
        auto r5 = gw.submit({ "BTCUSDT", true, 0.00024, 50000.0, false, "T" }); // would be $60
        CHECK(!r5.ok && ex.calls == 4); // gross cap
        auto rs = gw.submit({ "BTCUSDT", false, 0.00048, 50000.0, true, "T" }); // sell $24
        CHECK(rs.ok);
        CHECK(std::fabs(gw.pilot_gross_usd - 24.0) < 1e-6);
        auto r6 = gw.submit({ "BTCUSDT", true, 0.00024, 50000.0, false, "T" }); // fits again
        CHECK(r6.ok);
    }
    // 5. LIVE mode: EXIT on a NON-pilot symbol still passes (never blocked).
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        arm_pilot(gw);
        auto r = gw.submit({ "DOGEUSDT", false, 100.0, 1.0, /*is_exit*/true, "T" });
        CHECK(r.ok && ex.calls == 1 && !ex.last_buy);
    }
    // 6. pilot_enabled=false: LIVE behaves exactly as before (no scoping).
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        auto r = gw.submit({ "DOGEUSDT", true, 100.0, 1.0, false, "T" });
        CHECK(r.ok && ex.calls == 1);
    }

    if (fails == 0) { std::printf("pilot_scope_test: ALL PASS\n"); return 0; }
    std::printf("pilot_scope_test: %d FAILURE(S)\n", fails);
    return 1;
}

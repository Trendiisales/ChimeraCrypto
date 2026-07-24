// startup_reconcile_test.cpp — Phase-2 item 9.
// Proves: on restart, an open order found on the exchange is ADOPTED into the
// ledger + id-registry so the strategy does NOT resubmit it (no duplicate);
// a position mismatch BLOCKS trading; a clean match passes.
#include "live/StartupReconciler.hpp"
#include "live/ExecutionGateway.hpp"
#include "live/BinanceREST.hpp"   // reconstruct_avg_entry_from_body (pure static; no curl/HMAC called here)
#include <cstdio>
#include <cmath>
#include <string>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

struct MockExec {
    int calls=0;
    OrderResult execute(const std::string&, bool, double qty, double px, const std::string& = "") {
        ++calls; OrderResult r; r.ok=true; r.shadow=true; r.status="FILLED";
        r.executed_qty=qty; r.avg_price=px; return r;
    }
};

int main() {
    // --- restart with a working order: adopt, then prove no duplicate submit ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        OrderIdRegistry reg;
        StartupReconciler R;
        // The order the (now-restarted) RIP sleeve had already placed. Its id is
        // the DETERMINISTIC id RIP would regenerate for the same signal.
        std::string cid = OrderIdRegistry::make_client_id("RIP", 777, "SOLUSDT", true);
        ExchangeSnapshot snap; snap.ok = true;
        snap.open_orders.push_back({ cid, "SOLUSDT", "RIP", true, 5.0, 100.0 });
        // balances: none held yet (order still working) -> matches empty ledger
        auto res = R.reconcile(snap, L, &reg, /*now*/1000);
        CHECK(res.passed && res.adopted_orders == 1);
        CHECK(L.has_pending(cid) && reg.in_flight(cid));

        // The strategy re-fires the SAME signal after restart. Gateway must NOT
        // submit a duplicate — the id is already in flight -> recover first.
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        gw.set_ledger(&L); gw.set_id_registry(&reg);
        auto r = gw.submit({ "SOLUSDT", true, 5.0, 100.0, false, "RIP", /*signal_id*/777 });
        CHECK(!r.ok && r.error == "recover-first");
        CHECK(ex.calls == 0);                          // NO duplicate order
    }
    // --- position mismatch blocks trading ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        StartupReconciler R;
        ExchangeSnapshot snap; snap.ok = true;
        snap.base_balances["BTCUSDT"] = 1.5;           // exchange says 1.5 held
        // ledger thinks 0 -> mismatch -> must block
        auto res = R.reconcile(snap, L);
        CHECK(!res.passed && res.position_mismatches == 1);
    }
    // --- failed snapshot fetch blocks trading ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        StartupReconciler R; ExchangeSnapshot snap; snap.ok = false;
        CHECK(!R.reconcile(snap, L).passed);
    }
    // --- clean match passes ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        // ledger already holds 0.5 BTC; exchange agrees
        ExecReport r; r.client_id="x"; r.symbol="BTCUSDT"; r.source="RIP"; r.is_buy=true;
        r.state=OrderState::FILLED; r.filled_qty=0.5; r.avg_price=40000.0; L.apply_report(r);
        StartupReconciler R; ExchangeSnapshot snap; snap.ok=true;
        snap.base_balances["BTCUSDT"] = 0.5;
        CHECK(R.reconcile(snap, L).passed);
    }
    // ── PRE-BOOT HOLDINGS SEED (2026-07-24 native-stop residual) ──────────────
    // myTrades vwap reconstruction (pure parse). Oldest-first array; the current
    // lot is reconstructed from the MOST-RECENT buys back to held_qty. Sells ignored.
    {
        const std::string body =
          "[{\"symbol\":\"BTCUSDT\",\"id\":1,\"orderId\":1,\"price\":\"100.00\",\"qty\":\"1.00000000\","
            "\"quoteQty\":\"100.0\",\"commission\":\"0.1\",\"commissionAsset\":\"USDT\",\"time\":1,\"isBuyer\":true,\"isMaker\":false},"
           "{\"symbol\":\"BTCUSDT\",\"id\":2,\"orderId\":2,\"price\":\"110.00\",\"qty\":\"0.50000000\","
            "\"quoteQty\":\"55.0\",\"commission\":\"0.1\",\"commissionAsset\":\"USDT\",\"time\":2,\"isBuyer\":false,\"isMaker\":false},"
           "{\"symbol\":\"BTCUSDT\",\"id\":3,\"orderId\":3,\"price\":\"120.00\",\"qty\":\"2.00000000\","
            "\"quoteQty\":\"240.0\",\"commission\":\"0.1\",\"commissionAsset\":\"USDT\",\"time\":3,\"isBuyer\":true,\"isMaker\":false}]";
        // held 2.0 -> fully covered by the newest buy (2.0 @ 120) -> vwap 120
        CHECK(std::fabs(BinanceREST::reconstruct_avg_entry_from_body(body, 2.0) - 120.0) < 1e-6);
        // held 2.5 -> newest 2.0@120 + 0.5 of the old 1.0@100 -> (240+50)/2.5 = 116
        CHECK(std::fabs(BinanceREST::reconstruct_avg_entry_from_body(body, 2.5) - 116.0) < 1e-6);
        // no buys / empty -> 0 (caller falls back to a market anchor)
        CHECK(BinanceREST::reconstruct_avg_entry_from_body("[]", 1.0) == 0.0);
        CHECK(BinanceREST::reconstruct_avg_entry_from_body(body, 0.0) == 0.0);
    }
    // seed_position closes the pre-boot gap: a balance held before boot no longer
    // BLOCKS the reconcile (the mismatch case above) once seeded — and shows up in
    // held_symbols() so reconcile_stops() can arm a native stop on it.
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        double before_cash = L.total_cash();
        L.seed_position("ETHUSDT", 3.0, 2000.0);             // pre-boot hold, anchor 2000
        CHECK(L.position("ETHUSDT") == 3.0);
        CHECK(L.avg_price("ETHUSDT") == 2000.0);
        CHECK(L.total_cash() == before_cash);                // CASH-NEUTRAL (no double-charge)
        bool in_held = false;
        for (const auto& s : L.held_symbols()) if (s == "ETHUSDT") in_held = true;
        CHECK(in_held);
        StartupReconciler R; ExchangeSnapshot snap; snap.ok = true;
        snap.base_balances["ETHUSDT"] = 3.0;                 // exchange truth == seeded ledger
        CHECK(R.reconcile(snap, L).passed);                  // was a BLOCK before the seed
    }
    // stop-anchor clamp rule (mirrors main.cpp): anchor = min(entry, mkt) so an
    // underwater pre-boot hold can never arm a stop AT/above market.
    {
        auto clamp = [](double entry, double mkt){ double a = entry>0?entry:mkt; return a>mkt?mkt:a; };
        CHECK(clamp(2000.0, 1500.0) == 1500.0);              // underwater -> clamp to market
        CHECK(clamp(1000.0, 1500.0) == 1000.0);              // in profit  -> keep true entry
        CHECK(clamp(0.0,    1500.0) == 1500.0);              // unknown    -> market fallback
        // and the placed stop (anchor*(1-15%)) is strictly below market in every case
        CHECK(clamp(2000.0,1500.0)*0.85 < 1500.0);
        CHECK(clamp(1000.0,1500.0)*0.85 < 1500.0);
    }
    std::printf(fails==0 ? "PASS: restart adopt (no dup) / mismatch block / fetch-fail block / match pass / "
                           "pre-boot seed (vwap+cash-neutral+reconcile+clamp)\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

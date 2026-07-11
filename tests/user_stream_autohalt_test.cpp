// user_stream_autohalt_test.cpp — Phase-8G: USER-STREAM HEARTBEAT AUTO-HALT
// (the go-live blocker). LIVE-PATH test folded into the CI stale-user-stream
// matrix.
//
// Difference from stale_user_stream_test.cpp: that test drove the halt EXPLICITLY
// (it proved the mechanism exists). THIS test proves the halt AUTO-ARMS the
// instant a LIVE user-data-stream heartbeat lapses — via UserStreamHaltGuard
// polled from the gateway kill-switch — with NO explicit trigger, then AUTO-CLEARS
// on stream-resume + a clean reconcile. It also proves the SHADOW no-op: a
// shadow-driven stream never arms the halt.
//
// Composed from the real production primitives (no test-only logic):
//   * UserDataStream::heartbeat_lapsed()  — the live staleness signal.
//   * UserStreamHaltGuard::poll()/on_reconcile() — auto-arm latch + reconcile-clear.
//   * ExecutionGateway kill_switch_active — the entry chokepoint (exits pass).
//   * StartupReconciler::reconcile()      — the halt-clearing gate.
#include "live/UserDataStream.hpp"
#include "live/UserStreamHaltGuard.hpp"
#include "live/ExecutionGateway.hpp"
#include "live/StartupReconciler.hpp"
#include <cstdio>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

struct MockExec {
    int calls=0; bool last_buy=false;
    OrderResult execute(const std::string&, bool is_buy, double qty, double px, const std::string& = "") {
        ++calls; last_buy=is_buy; OrderResult r; r.ok=true; r.shadow=true; r.status="FILLED";
        r.executed_qty=qty; r.avg_price=px; return r;
    }
};

int main() {
    const int64_t THRESH = 45'000;   // 45s

    // ═══════════════════════════════════════════════════════════════════════
    // PART A — SHADOW NO-OP: a shadow-driven stream never arms the halt even if
    // the heartbeat clock is ancient (there is no live stream to lapse).
    // ═══════════════════════════════════════════════════════════════════════
    {
        UserDataStream shadow_stream;                 // shadow_driven_ = true (default)
        shadow_stream.set_listen_key("lk-shadow", 0);
        UserStreamHaltGuard g; g.set_threshold_ms(THRESH);
        // Even a huge elapsed time does NOT arm a shadow-driven stream.
        CHECK(!shadow_stream.heartbeat_lapsed(10'000'000, THRESH));
        CHECK(!g.poll(shadow_stream, 10'000'000));
        CHECK(!g.halted() && g.arm_count() == 0);     // inert in shadow
    }

    // ═══════════════════════════════════════════════════════════════════════
    // PART B — LIVE PATH: auto-arm on lapse → entries blocked / exits pass →
    // stays blocked on a mismatched reconcile → resume + clean reconcile clears.
    // ═══════════════════════════════════════════════════════════════════════
    ExchangeLedger L; L.configure(/*cash*/100000.0, /*enforce*/false, /*fee*/0.001);
    UserDataStream stream;
    stream.set_shadow_driven(false);                  // GO-LIVE: real WS user-stream
    int64_t t0 = 1'000'000;
    stream.set_listen_key("lk-live", t0);             // stream established, heartbeat fresh
    stream.mark_heartbeat(t0);

    UserStreamHaltGuard guard; guard.set_threshold_ms(THRESH);

    MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
    // Wire the guard into the ONE kill-switch EXACTLY as production does — the
    // gateway polls the guard against the live stream at each entry gate.
    int64_t now = t0;
    gw.kill_switch_active = [&]{ return guard.poll(stream, now); };

    // 1. Heartbeat FRESH => no auto-halt; an entry passes.
    now = t0 + 20'000;                                 // 20s < 45s
    stream.mark_heartbeat(t0 + 20'000);                // stream still delivering events
    CHECK(!guard.poll(stream, now));
    auto e0 = gw.submit({ "BTCUSDT", true, 0.01, 40000.0, /*is_exit*/false, "RIP" });
    CHECK(e0.ok && ex.calls == 1);                     // entry permitted (stream healthy)

    // 2. Stream goes SILENT: no heartbeat for > threshold. The NEXT entry gate
    //    AUTO-ARMS the halt with NO explicit trigger.
    now = (t0 + 20'000) + THRESH + 1;                  // 45.001s since last heartbeat
    CHECK(stream.heartbeat_lapsed(now, THRESH));       // the live staleness signal
    auto e1 = gw.submit({ "BTCUSDT", true, 0.01, 40000.0, false, "RIP" });
    CHECK(!e1.ok && ex.calls == 1);                    // ENTRY auto-HALTED
    CHECK(guard.halted() && guard.arm_count() == 1);   // latch armed by the poll

    // 3. EXITS are NEVER blocked, even while halted.
    auto x1 = gw.submit({ "BTCUSDT", false, 0.01, 40000.0, /*is_exit*/true, "RIP" });
    CHECK(x1.ok && ex.calls == 2 && !x1.error.size()); // exit passes

    // 4. A late stray heartbeat does NOT self-clear the latch — only a reconcile
    //    can (holdings may have drifted while the stream was down).
    stream.mark_heartbeat(now);                        // stream twitched back
    CHECK(guard.halted());                             // still latched
    auto e2 = gw.submit({ "BTCUSDT", true, 0.01, 40000.0, false, "RIP" });
    CHECK(!e2.ok && ex.calls == 2);                    // STILL halted (no reconcile yet)

    // 5. A MISMATCHED reconcile does NOT clear the halt.
    StartupReconciler R;
    ExchangeSnapshot bad; bad.ok = true; bad.base_balances["BTCUSDT"] = 1.5;  // exch 1.5, ledger 0
    auto rb = R.reconcile(bad, L);
    CHECK(!rb.passed && rb.position_mismatches == 1);
    guard.on_reconcile(rb.passed);
    CHECK(guard.halted());                             // stays armed
    auto e3 = gw.submit({ "BTCUSDT", true, 0.01, 40000.0, false, "RIP" });
    CHECK(!e3.ok && ex.calls == 2);                    // still blocked

    // 6. Stream RESUMED (heartbeat fresh) + a CLEAN reconcile => AUTO-CLEARS.
    now += 5'000;
    stream.mark_heartbeat(now);                        // healthy again
    CHECK(!stream.heartbeat_lapsed(now, THRESH));
    ExchangeSnapshot good; good.ok = true;             // ledger 0, exchange agrees
    auto rg = R.reconcile(good, L);
    CHECK(rg.passed);
    guard.on_reconcile(rg.passed);
    CHECK(!guard.halted() && guard.clear_count() == 1);// latch cleared
    auto e4 = gw.submit({ "ETHUSDT", true, 0.5, 2000.0, false, "RIP" });
    CHECK(e4.ok && ex.calls == 3);                     // entries resume post-reconcile

    std::printf(fails==0
        ? "PASS: user-stream heartbeat AUTO-HALT (lapse->block, exits pass, stays until clean reconcile->clear); shadow no-op\n"
        : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

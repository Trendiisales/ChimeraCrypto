// stale_user_stream_test.cpp — permanent CI matrix item: STALE-USER-STREAM
// (halt-until-reconcile).
//
// The user-data stream is the authoritative execution-report path. If it goes
// STALE (a missed listenKey keepalive => the stream is considered dropped and
// holdings may have drifted from the exchange's truth), the desk MUST halt NEW
// entries and stay halted UNTIL a StartupReconciler pass re-agrees the ledger
// with the exchange. Risk-reducing EXITS are never blocked.
//
// This test COMPOSES the existing primitives that implement that guarantee — it
// adds NO engine logic:
//   * UserDataStream::needs_keepalive()  — the staleness signal (keepalive missed
//     past the renewal interval => treat the stream as dropped/stale).
//   * ExecutionGateway kill_switch_active — the entry chokepoint (entries blocked
//     while active, exits always pass).
//   * StartupReconciler::reconcile()      — the halt-clearing gate (a position
//     mismatch keeps trading blocked; a clean match permits resume).
//
// NOTE (honest go-live gap, same as the phase notes): the LIVE loop that AUTO-
// arms this kill-switch the instant the stream heartbeat lapses is a documented
// wiring follow-up; here the composition is driven explicitly so the mechanism
// is pinned and provably PASSES against current behaviour.
#include "live/UserDataStream.hpp"
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
    ExchangeLedger L; L.configure(/*cash*/100000.0, /*enforce*/false, /*fee*/0.001);
    UserDataStream stream;
    bool reconciled = false;

    // The stream was healthy; then the keepalive lapses -> STALE (proxy for a drop).
    int64_t t0 = 1'000'000;
    stream.set_listen_key("lk-1", t0);
    CHECK(stream.active());
    CHECK(!stream.needs_keepalive(t0 + 29*60*1000));            // still fresh
    int64_t t_stale = t0 + 31*60*1000;
    CHECK(stream.needs_keepalive(t_stale));                     // STALE now

    // A stale stream that has NOT been re-reconciled halts entries.
    auto stream_halted = [&]{ return stream.needs_keepalive(t_stale) && !reconciled; };

    MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
    gw.kill_switch_active = [&]{ return stream_halted(); };

    // 1. While stale + not reconciled: ENTRY blocked, EXIT still permitted.
    CHECK(stream_halted());
    auto e1 = gw.submit({ "BTCUSDT", true, 0.01, 40000.0, /*is_exit*/false, "RIP" });
    CHECK(!e1.ok && ex.calls == 0);                             // entry HALTED
    auto x1 = gw.submit({ "BTCUSDT", false, 0.01, 40000.0, /*is_exit*/true,  "RIP" });
    CHECK(x1.ok && ex.calls == 1 && !x1.error.size());          // exit NEVER blocked

    // 2. A reconcile with a POSITION MISMATCH does NOT clear the halt.
    StartupReconciler R;
    ExchangeSnapshot bad; bad.ok = true; bad.base_balances["BTCUSDT"] = 1.5;  // exch says 1.5, ledger 0
    auto rb = R.reconcile(bad, L);
    CHECK(!rb.passed && rb.position_mismatches == 1);
    reconciled = rb.passed;                                     // stays false
    CHECK(stream_halted());
    auto e2 = gw.submit({ "BTCUSDT", true, 0.01, 40000.0, false, "RIP" });
    CHECK(!e2.ok && ex.calls == 1);                             // STILL halted (no new entry)

    // 3. A CLEAN reconcile clears the halt; keepalive renewed; entries resume.
    ExchangeSnapshot good; good.ok = true;                      // ledger holds 0, exch agrees (empty)
    auto rg = R.reconcile(good, L);
    CHECK(rg.passed);
    reconciled = rg.passed;
    stream.mark_keepalive(t_stale);                            // stream healthy again
    CHECK(!stream_halted());
    auto e3 = gw.submit({ "ETHUSDT", true, 0.5, 2000.0, false, "RIP" });
    CHECK(e3.ok && ex.calls == 2);                              // entry permitted post-reconcile

    std::printf(fails==0
        ? "PASS: stale user-stream halts entries (exits pass) until a clean reconcile resumes\n"
        : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

// user_stream_test.cpp — Phase-2 item 8.
// Proves: the user-data-stream code path drives the ledger from execution
// reports (in shadow, from simulated reports); listenKey renewal timing works;
// live-connected is honestly false in shadow.
#include "live/UserDataStream.hpp"
#include "live/ExchangeLedger.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
static bool near(double a,double b,double e=1e-6){ return std::fabs(a-b)<e; }

int main() {
    ExchangeLedger L; L.configure(0.0, false, 0.001);   // track-only
    UserDataStream s;
    s.set_handler([&](const ExecReport& r){ L.apply_report(r); });

    // A simulated FILLED buy report flows through the stream -> ledger updates.
    ExecReport r; r.client_id="ABC"; r.symbol="BTCUSDT"; r.source="RIP"; r.is_buy=true;
    r.state=OrderState::FILLED; r.filled_qty=0.5; r.avg_price=40000.0;
    s.feed_report(r);
    CHECK(near(L.position("BTCUSDT"), 0.5));
    CHECK(s.reports() == 1);

    // A FILLED sell report reduces the position.
    ExecReport x; x.client_id="DEF"; x.symbol="BTCUSDT"; x.source="RIP"; x.is_buy=false;
    x.state=OrderState::FILLED; x.filled_qty=0.5; x.avg_price=41000.0;
    s.feed_report(x);
    CHECK(near(L.position("BTCUSDT"), 0.0));

    // listenKey renewal (< 30 min) lifecycle.
    int64_t t0 = 1000000;
    s.set_listen_key("listenkey-xyz", t0);
    CHECK(s.active());
    CHECK(!s.needs_keepalive(t0 + 29*60*1000));         // not yet
    CHECK(s.needs_keepalive(t0 + 31*60*1000));          // due
    s.mark_keepalive(t0 + 31*60*1000);
    CHECK(!s.needs_keepalive(t0 + 31*60*1000 + 60*1000));

    // Honest status: shadow-driven => not live-connected.
    CHECK(!s.live_connected());

    std::printf(fails==0 ? "PASS: stream drives ledger + listenKey renewal + honest status\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

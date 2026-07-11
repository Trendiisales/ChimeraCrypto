// exchange_filters_test.cpp — Phase-2 item 5.
// Proves: exchangeInfo filters cache + normalize (step/min-qty/min-notional);
// a filter CHANGE refreshes the cache and a now-sub-min order is rejected.
#include "live/ExchangeFilters.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
static bool near(double a,double b,double e=1e-9){ return std::fabs(a-b)<e; }

int main() {
    ExchangeFilters F;
    // --- programmatic set ---
    SymbolFilter sf; sf.step_size=0.01; sf.min_qty=0.01; sf.min_notional=5.0; sf.qty_prec=2;
    F.set("SOLUSDT", sf);

    auto n1 = F.normalize("SOLUSDT", 3.456, 100.0);   // -> floor to step 3.45
    CHECK(n1.ok && near(n1.qty, 3.45));
    auto n2 = F.normalize("SOLUSDT", 0.005, 100.0);   // rounds below step -> reject
    CHECK(!n2.ok);
    auto n3 = F.normalize("SOLUSDT", 0.02, 100.0);    // $2 < min-notional 5 -> reject
    CHECK(!n3.ok && n3.reason.find("NOTIONAL") != std::string::npos);
    auto n4 = F.normalize("UNKNOWNUSDT", 1.234, 10.0);// no cached filter -> pass-through
    CHECK(n4.ok);

    // --- JSON load (exchangeInfo shape) ---
    std::string info = R"({"symbols":[
      {"symbol":"BTCUSDT","baseAssetPrecision":8,"filters":[
        {"filterType":"LOT_SIZE","minQty":"0.00001000","maxQty":"9000.0","stepSize":"0.00001000"},
        {"filterType":"MARKET_LOT_SIZE","stepSize":"0.00001000"},
        {"filterType":"NOTIONAL","minNotional":"5.00000000"}]},
      {"symbol":"ADAUSDT","baseAssetPrecision":8,"filters":[
        {"filterType":"LOT_SIZE","minQty":"0.10000000","maxQty":"90000.0","stepSize":"0.10000000"},
        {"filterType":"NOTIONAL","minNotional":"5.00000000"}]}]})";
    int loaded = F.load_from_json(info);
    CHECK(loaded == 2);
    CHECK(F.has("BTCUSDT") && F.has("ADAUSDT"));
    const SymbolFilter* b = F.get("BTCUSDT");
    CHECK(b && near(b->step_size, 0.00001) && near(b->min_notional, 5.0));

    auto a1 = F.normalize("ADAUSDT", 12.34, 100.0);   // step 0.1 -> 12.3, $1230 ok
    CHECK(a1.ok && near(a1.qty, 12.3));

    // --- filter CHANGE: reload with a tighter min-notional; cache must refresh ---
    std::string info2 = R"({"symbols":[
      {"symbol":"ADAUSDT","baseAssetPrecision":8,"filters":[
        {"filterType":"LOT_SIZE","minQty":"0.10000000","maxQty":"90000.0","stepSize":"0.10000000"},
        {"filterType":"NOTIONAL","minNotional":"50.00000000"}]}]})";
    F.load_from_json(info2);
    const SymbolFilter* a = F.get("ADAUSDT");
    CHECK(a && near(a->min_notional, 50.0));           // refreshed
    auto a2 = F.normalize("ADAUSDT", 0.3, 100.0);      // $30 < new min-notional 50 -> reject
    CHECK(!a2.ok && a2.reason.find("NOTIONAL") != std::string::npos);
    auto a3 = F.normalize("ADAUSDT", 0.6, 100.0);      // $60 >= 50 -> ok
    CHECK(a3.ok);

    std::printf(fails==0 ? "PASS: filters cache/normalize + change-refresh reject\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

// order_recovery_test.cpp — Phase-2 item 7.
// Proves: deterministic client ids; after a timeout the registry demands a
// query-by-id (RECOVER) and NEVER a blind resubmit; when the query finds the
// order, no duplicate buy is issued.
#include "live/OrderIdRegistry.hpp"
#include <cstdio>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

int main() {
    // deterministic: identical intent -> identical id
    std::string a = OrderIdRegistry::make_client_id("RIP", 12345, "solusdt", true);
    std::string b = OrderIdRegistry::make_client_id("RIP", 12345, "SOLUSDT", true);
    CHECK(a == b && a.size() <= 36 && !a.empty());
    std::string c = OrderIdRegistry::make_client_id("RIP", 12346, "SOLUSDT", true);
    CHECK(c != a);                                   // different signal -> different id

    OrderIdRegistry reg; reg.set_timeout_ms(5000);
    int64_t t0 = 1000;
    CHECK(reg.on_submit(a, t0) == RecoveryAction::SAFE_TO_SUBMIT);
    // resubmit while in flight -> must recover, not blind resubmit
    CHECK(reg.on_submit(a, t0+10) == RecoveryAction::RECOVER_QUERY_FIRST);

    // timeout after accept -> UNKNOWN -> query first
    reg.on_timeout(a, t0+6000);
    CHECK(reg.decide(a, t0+6001) == RecoveryAction::RECOVER_QUERY_FIRST);

    // query finds the order EXISTS -> do NOT resubmit (no duplicate buy)
    CHECK(reg.on_query_result(a, /*found*/true) == RecoveryAction::WAIT);
    CHECK(reg.in_flight(a));

    // separate id: query finds it ABSENT -> safe to (re)submit
    OrderIdRegistry reg2;
    reg2.on_submit(c, t0);
    reg2.on_timeout(c, t0+6000);
    CHECK(reg2.decide(c, t0+6001) == RecoveryAction::RECOVER_QUERY_FIRST);
    CHECK(reg2.on_query_result(c, /*found*/false) == RecoveryAction::SAFE_TO_SUBMIT);
    CHECK(!reg2.in_flight(c));

    std::printf(fails==0 ? "PASS: deterministic ids + query-first recovery, no dup\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

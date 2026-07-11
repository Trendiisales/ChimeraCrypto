// runtime_mode_test.cpp — Phase-1 regression test for the unified runtime mode.
// Proves conflicting mode signals => resolution fails (startup would ABORT).
// Build: g++ -std=c++20 -I../include runtime_mode_test.cpp -o runtime_mode_test
#include "live/RuntimeMode.hpp"
#include <cstdio>
using namespace chimera;

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

int main() {
    RuntimeMode m;
    // parsing
    CHECK(parse_runtime_mode("live", m)  && m == RuntimeMode::LIVE);
    CHECK(parse_runtime_mode(" Shadow ", m) && m == RuntimeMode::SHADOW);
    CHECK(parse_runtime_mode("DISABLED", m) && m == RuntimeMode::DISABLED);
    CHECK(!parse_runtime_mode("banana", m));

    // No explicit mode key: derive from shadow_mode bool.
    CHECK(resolve_runtime_mode(true,  false, RuntimeMode::SHADOW).ok);
    CHECK(resolve_runtime_mode(true,  false, RuntimeMode::SHADOW).mode == RuntimeMode::SHADOW);
    CHECK(resolve_runtime_mode(false, false, RuntimeMode::SHADOW).mode == RuntimeMode::LIVE);

    // Explicit mode agreeing with shadow_mode: ok.
    CHECK(resolve_runtime_mode(true,  true, RuntimeMode::SHADOW).ok);
    CHECK(resolve_runtime_mode(false, true, RuntimeMode::LIVE).ok);

    // CONFLICT: mode=LIVE but shadow_mode=true  => resolution fails (would abort).
    auto c1 = resolve_runtime_mode(true, true, RuntimeMode::LIVE);
    CHECK(!c1.ok);
    std::printf("  conflict1: %s\n", c1.error.c_str());

    // CONFLICT: mode=SHADOW but shadow_mode=false => fails.
    auto c2 = resolve_runtime_mode(false, true, RuntimeMode::SHADOW);
    CHECK(!c2.ok);
    std::printf("  conflict2: %s\n", c2.error.c_str());

    // Cross-file check vs credentials shadow flag.
    CHECK(cross_check_credentials_shadow(RuntimeMode::SHADOW, true).ok);   // shadow + creds-shadow: ok
    CHECK(cross_check_credentials_shadow(RuntimeMode::LIVE,  false).ok);   // live + creds-live: ok
    auto c3 = cross_check_credentials_shadow(RuntimeMode::LIVE, true);     // live but creds say shadow
    CHECK(!c3.ok);
    std::printf("  conflict3: %s\n", c3.error.c_str());
    auto c4 = cross_check_credentials_shadow(RuntimeMode::SHADOW, false);  // shadow but creds say live
    CHECK(!c4.ok);
    std::printf("  conflict4: %s\n", c4.error.c_str());

    std::printf(fails == 0 ? "PASS: runtime mode resolution + conflict aborts\n" : "FAILED (%d)\n", fails);
    return fails == 0 ? 0 : 1;
}

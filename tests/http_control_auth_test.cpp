// http_control_auth_test.cpp — Phase-1 regression test for control-API auth.
// Proves: unauthenticated kill/reset => rejected; non-POST => rejected;
// correct token (header or query) => authorized.
// Build: g++ -std=c++20 -I../include http_control_auth_test.cpp -o http_control_auth_test
#include "live/HttpControlAuth.hpp"
#include <cstdio>
using namespace chimera;

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

int main() {
    const std::string TOKEN = "s3cr3t-token";

    // Method detection
    CHECK( http_request_is_post("POST /api/kill HTTP/1.1\r\n\r\n"));
    CHECK(!http_request_is_post("GET /api/kill HTTP/1.1\r\n\r\n"));

    // Mutating endpoint detection
    CHECK(http_is_mutating_control("POST /api/kill HTTP/1.1\r\n"));
    CHECK(http_is_mutating_control("POST /api/session_reset HTTP/1.1\r\n"));
    CHECK(http_is_mutating_control("POST /api/ratchet_reset HTTP/1.1\r\n"));
    CHECK(http_is_mutating_control("POST /api/daily_kill_clear HTTP/1.1\r\n"));
    CHECK(!http_is_mutating_control("GET /api/state HTTP/1.1\r\n"));
    CHECK(!http_is_mutating_control("GET /api/positions HTTP/1.1\r\n"));

    // Unauthenticated kill => rejected
    const char* no_auth = "POST /api/kill HTTP/1.1\r\nHost: x\r\n\r\n";
    CHECK(!http_control_authorized(no_auth, TOKEN));

    // Wrong token => rejected
    const char* wrong = "POST /api/kill HTTP/1.1\r\nX-Auth-Token: nope\r\n\r\n";
    CHECK(!http_control_authorized(wrong, TOKEN));

    // Correct token via header => authorized
    const char* hdr = "POST /api/kill HTTP/1.1\r\nX-Auth-Token: s3cr3t-token\r\n\r\n";
    CHECK(http_control_authorized(hdr, TOKEN));

    // Correct token via query param => authorized
    const char* qry = "POST /api/session_reset?token=s3cr3t-token HTTP/1.1\r\n\r\n";
    CHECK(http_control_authorized(qry, TOKEN));

    // No server token configured => always deny (fail closed)
    CHECK(!http_control_authorized(hdr, ""));

    // Simulate the server guard: mutating + (non-POST OR unauth) => reject.
    auto server_rejects = [&](const char* req) {
        if (!http_is_mutating_control(req)) return false;   // not a guarded endpoint
        if (!http_request_is_post(req)) return true;        // 405
        if (!http_control_authorized(req, TOKEN)) return true; // 401
        return false;
    };
    CHECK( server_rejects("GET /api/kill HTTP/1.1\r\n\r\n"));                       // non-POST
    CHECK( server_rejects("POST /api/kill HTTP/1.1\r\n\r\n"));                      // unauth
    CHECK(!server_rejects("POST /api/kill HTTP/1.1\r\nX-Auth-Token: s3cr3t-token\r\n\r\n")); // ok
    CHECK(!server_rejects("GET /api/state HTTP/1.1\r\n\r\n"));                      // read endpoint fine

    std::printf(fails == 0 ? "PASS: control-API auth + method enforcement\n" : "FAILED (%d)\n", fails);
    return fails == 0 ? 0 : 1;
}

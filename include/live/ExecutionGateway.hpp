#pragma once
// ============================================================================
// ExecutionGateway — THE single chokepoint for every strategy order.
//
// Phase-1 review fix (2026-07-11): strategy code called SpotExecutor::execute()
// directly from 5 sites, 2 of which (XSec rebalance + RipRider sleeve) bypassed
// ALL risk machinery. This gateway is now the ONLY path to the executor:
// SpotExecutor::execute is private and befriends ExecutionGatewayT, so a direct
// strategy call no longer compiles.
//
// submit() applies, in order:
//   1. MODE gate       — DISABLED never trades; only LIVE reaches a real POST
//   2. sanity/filters  — qty>0, ref_px>0, min-notional (exchange MIN_NOTIONAL)
//   3. KILL SWITCH     — ENTRIES blocked while a daily-loss / emergency halt is
//                        active. EXITS (risk-reducing sells) are NEVER blocked.
//   4. exposure hook   — optional caller-supplied cap check (entries only)
// then, and only then, forwards to the (befriended) executor.
//
// Templated on the executor type so it is unit-testable with a mock while the
// production alias ExecutionGatewayT<SpotExecutor> retains friend access.
// ============================================================================
#include <string>
#include <functional>
#include <cstdio>
#include "live/BinanceREST.hpp"   // OrderResult
#include "live/RuntimeMode.hpp"

namespace chimera {

struct OrderIntent {
    std::string symbol;          // exchange symbol, e.g. "BTCUSDT" or "btcusdt"
    bool        is_buy  = true;
    double      qty     = 0.0;   // base-asset quantity
    double      ref_px  = 0.0;   // signal / reference price
    bool        is_exit = false; // true => risk-reducing; never blocked by halts
    const char* source  = "?";   // engine / sleeve tag (logging only)
};

template <class Exec>
class ExecutionGatewayT {
public:
    ExecutionGatewayT(Exec& ex, RuntimeMode mode) : ex_(ex), mode_(mode) {}

    // Entries are rejected while this returns true (daily-loss / emergency halt).
    std::function<bool()> kill_switch_active;
    // Optional: return false to reject an ENTRY that would breach exposure caps.
    std::function<bool(const OrderIntent&)> exposure_ok;

    RuntimeMode mode() const { return mode_; }
    double min_notional_usd = 5.0;   // Binance spot MIN_NOTIONAL floor

    OrderResult submit(const OrderIntent& in) {
        OrderResult r;
        // 1. MODE
        if (mode_ == RuntimeMode::DISABLED) {
            log_reject(in, "mode=DISABLED"); r.error = "mode=DISABLED"; return r;
        }
        // 2. sanity / exchange filters
        if (in.qty <= 0.0 || in.ref_px <= 0.0) {
            log_reject(in, "invalid qty/price"); r.error = "invalid qty/price"; return r;
        }
        if (!in.is_exit && (in.qty * in.ref_px) < min_notional_usd) {
            log_reject(in, "below MIN_NOTIONAL"); r.error = "below min notional"; return r;
        }
        // 3. KILL SWITCH — entries only; a risk-reducing exit is never blocked
        if (!in.is_exit && kill_switch_active && kill_switch_active()) {
            log_reject(in, "kill-switch/halt active"); r.error = "halt active"; return r;
        }
        // 4. exposure cap — entries only
        if (!in.is_exit && exposure_ok && !exposure_ok(in)) {
            log_reject(in, "exposure cap"); r.error = "exposure cap"; return r;
        }
        // Forward to the executor (befriended private execute()).
        return ex_.execute(in.symbol, in.is_buy, in.qty, in.ref_px);
    }

private:
    void log_reject(const OrderIntent& in, const char* why) {
        std::fprintf(stderr, "[GATEWAY] REJECT src=%s %s %s qty=%.8f px=%.6f — %s\n",
                     in.source ? in.source : "?", in.is_buy ? "BUY" : "SELL",
                     in.symbol.c_str(), in.qty, in.ref_px, why);
    }
    Exec&       ex_;
    RuntimeMode mode_;
};

} // namespace chimera

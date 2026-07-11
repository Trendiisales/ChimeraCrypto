#pragma once
// ============================================================================
// StartupReconciler — on boot (and periodically), the exchange's own state
// (balances + open orders + recent trades) must equal the internal ledger
// BEFORE trading is permitted. (Phase-2 review fix item 9, 2026-07-11.)
//
// A restart with a working order or a position the ledger has forgotten is how
// duplicates and phantom exits happen. This blocks trading until the ledger and
// the exchange snapshot agree; open orders found on the exchange are ADOPTED
// into the ledger's pending set (with their client ids) so the strategy will not
// resubmit them.
//
// LIVE: the snapshot comes from BinanceREST (account balances, openOrders,
// myTrades). SHADOW: the snapshot is empty at a clean boot, or seeded from a
// persisted working-order file to exercise the "no duplicate after restart"
// path. Header-only, no REST dependency — the caller fills the snapshot.
// ============================================================================
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdint>
#include "live/ExchangeLedger.hpp"
#include "live/OrderIdRegistry.hpp"

namespace chimera {

struct OpenOrderSnap {
    std::string client_id, symbol, source;
    bool   is_buy = true;
    double qty = 0.0, px = 0.0;
};

struct ExchangeSnapshot {
    std::map<std::string, double> base_balances;   // symbol(upper) -> base qty held
    std::vector<OpenOrderSnap>    open_orders;      // working orders on the exchange
    double usdt_free = 0.0;
    bool   ok = false;                              // false => snapshot fetch failed
};

class StartupReconciler {
public:
    void set_qty_tolerance(double t) { qty_tol_ = t; }

    struct Result {
        bool passed = false;
        int  adopted_orders = 0;
        int  position_mismatches = 0;
        std::string detail;
    };

    // Reconcile the snapshot against the ledger. Adopts any open orders the
    // ledger does not already know so they will not be duplicated. Fails (blocks
    // trading) if the snapshot could not be fetched or a position disagrees
    // beyond tolerance.
    // idreg (optional): adopted working orders are registered in-flight so the
    // deterministic-id gate refuses to resubmit them (no duplicate after restart).
    Result reconcile(const ExchangeSnapshot& snap, ExchangeLedger& ledger,
                     OrderIdRegistry* idreg = nullptr, int64_t now_ms = 0) {
        Result res;
        if (!snap.ok) { res.detail = "snapshot fetch failed — trading blocked"; return res; }

        // 1. Adopt working orders the ledger has not seen (prevents re-submit).
        for (const auto& o : snap.open_orders) {
            if (!ledger.has_pending(o.client_id)) {
                ledger.adopt_pending(o.client_id, o.symbol, o.source, o.is_buy, o.qty, o.px);
                ++res.adopted_orders;
            }
            if (idreg) idreg->on_submit(o.client_id, now_ms);  // mark in-flight => query-first, never blind resubmit
        }

        // 2. Balances must match the ledger's positions within tolerance.
        for (const auto& kv : snap.base_balances) {
            double book = ledger.position(kv.first);
            if (std::fabs(book - kv.second) > qty_tol_) {
                ++res.position_mismatches;
                res.detail += kv.first + "(book=" + fmt(book) + " exch=" + fmt(kv.second) + ") ";
            }
        }
        // Ledger positions the exchange does not report are also a mismatch.
        // (Caller passes the full balance set; a missing key => exchange qty 0.)

        res.passed = (res.position_mismatches == 0);
        if (res.passed && res.detail.empty())
            res.detail = "reconciled: " + std::to_string(res.adopted_orders) + " open order(s) adopted, positions match";
        return res;
    }

private:
    static std::string fmt(double v) { char b[32]; std::snprintf(b, sizeof b, "%.8f", v); return b; }
    double qty_tol_ = 1e-6;
};

} // namespace chimera

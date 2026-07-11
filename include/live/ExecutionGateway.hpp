#pragma once
// ============================================================================
// ExecutionGateway — THE single chokepoint for every strategy order.
//
// Phase-1 (2026-07-11): SpotExecutor::execute() is private + befriends this
// gateway; all strategy orders route through submit(OrderIntent). The gateway
// enforced mode + kill-switch + min-notional.
//
// Phase-2 (2026-07-11) — EXCHANGE TRUTH. The gateway now also, in order:
//   * TIME SYNC   — refuse a SIGNED (LIVE) entry while the clock is out of
//                   tolerance (Binance -1021). Exits are never blocked.  [item 6]
//   * FILTERS     — snap qty to LOT_SIZE/MARKET_LOT_SIZE/MIN_NOTIONAL/step/
//                   precision BEFORE approval; reject a genuinely sub-min order. [item 5]
//   * CASH RESV   — reserve cash for a BUY before submission; REJECT or RESIZE
//                   when capital is insufficient (kills cross-sleeve overbook). [item 2]
//   * CLIENT ID   — mint a DETERMINISTIC client id + consult the recovery
//                   registry so an ambiguous order is queried, never blind-
//                   resubmitted.                                            [item 7]
//   * LEDGER      — after the executor returns, drive the authoritative ledger
//                   from the resulting execution report (via the user-data
//                   stream path when present) — holdings update ONLY from
//                   reports, never from intent.                        [items 1,3,8]
//
// Every Phase-2 component is an OPTIONAL attachment (null => Phase-1 behaviour),
// so the gateway is still unit-testable with a bare mock executor.
// ============================================================================
#include <string>
#include <functional>
#include <cstdio>
#include <chrono>
#include <cstdint>
#include "live/BinanceREST.hpp"   // OrderResult
#include "live/RuntimeMode.hpp"
#include "live/ExchangeLedger.hpp"
#include "live/ExchangeFilters.hpp"
#include "live/ExchangeTimeSync.hpp"
#include "live/OrderIdRegistry.hpp"
#include "live/UserDataStream.hpp"

namespace chimera {

struct OrderIntent {
    std::string symbol;          // exchange symbol, e.g. "BTCUSDT" or "btcusdt"
    bool        is_buy  = true;
    double      qty     = 0.0;   // base-asset quantity
    double      ref_px  = 0.0;   // signal / reference price
    bool        is_exit = false; // true => risk-reducing; never blocked by halts
    const char* source  = "?";   // engine / sleeve tag (logging + attribution)
    uint64_t    signal_id = 0;   // unique per intended order; 0 => id-recovery gate off
};

template <class Exec>
class ExecutionGatewayT {
public:
    ExecutionGatewayT(Exec& ex, RuntimeMode mode) : ex_(ex), mode_(mode) {}

    // Entries are rejected while this returns true (daily-loss / emergency halt).
    std::function<bool()> kill_switch_active;
    // Optional: return false to reject an ENTRY that would breach exposure caps.
    std::function<bool(const OrderIntent&)> exposure_ok;
    // Phase-4 item 22: OPTIONAL observer fired after a successful fill (never
    // affects routing). Feeds the additive realistic-fill parallel book. Args:
    // (intent, filled_qty, signal/ref px). null => no-op. Grid companions never
    // route through the gateway, so this can NOT touch the grid's own book.
    std::function<void(const OrderIntent&, double, double)> on_fill_observer;

    // Phase-2 optional attachments (null => skipped).
    void set_ledger(ExchangeLedger* l)     { ledger_ = l; }
    void set_filters(ExchangeFilters* f)   { filters_ = f; }
    void set_clock(ExchangeTimeSync* c)    { clock_ = c; }
    void set_id_registry(OrderIdRegistry* r) { idreg_ = r; }
    void set_stream(UserDataStream* s)     { stream_ = s; }
    ExchangeLedger* ledger() const { return ledger_; }

    RuntimeMode mode() const { return mode_; }
    double min_notional_usd = 5.0;   // Binance spot MIN_NOTIONAL floor

    OrderResult submit(const OrderIntent& in) {
        OrderResult r;
        // 1. MODE
        if (mode_ == RuntimeMode::DISABLED) {
            log_reject(in, "mode=DISABLED"); r.error = "mode=DISABLED"; return r;
        }
        // 2. TIME SYNC — a SIGNED (live) entry must not fire on a drifting clock.
        //    Exits (risk-reducing) are never blocked.
        if (clock_ && mode_ == RuntimeMode::LIVE && !in.is_exit && clock_->signed_trading_halted()) {
            log_reject(in, "clock-drift halt"); r.error = "clock-drift halt"; return r;
        }
        // 3. sanity
        if (in.qty <= 0.0 || in.ref_px <= 0.0) {
            log_reject(in, "invalid qty/price"); r.error = "invalid qty/price"; return r;
        }
        // 4. EXCHANGE FILTERS — normalize qty before approval.
        double qty = in.qty;
        if (filters_) {
            NormalizedOrder n = filters_->normalize(in.symbol, qty, in.ref_px, /*market*/true);
            if (!n.ok) {
                std::string why = "filter: " + n.reason;
                log_reject(in, why.c_str()); r.error = why; return r;
            }
            qty = n.qty;
        }
        // 5. min-notional (entries)
        if (!in.is_exit && (qty * in.ref_px) < min_notional_usd) {
            log_reject(in, "below MIN_NOTIONAL"); r.error = "below min notional"; return r;
        }
        // 6. KILL SWITCH — entries only.
        if (!in.is_exit && kill_switch_active && kill_switch_active()) {
            log_reject(in, "kill-switch/halt active"); r.error = "halt active"; return r;
        }
        // 7. exposure cap — entries only.
        if (!in.is_exit && exposure_ok && !exposure_ok(in)) {
            log_reject(in, "exposure cap"); r.error = "exposure cap"; return r;
        }
        // 8. DETERMINISTIC client id + recovery gate.
        int64_t now = now_ms();
        std::string cid = OrderIdRegistry::make_client_id(
            in.source ? in.source : "?", in.signal_id, in.symbol, in.is_buy);
        if (idreg_ && in.signal_id != 0) {
            RecoveryAction act = idreg_->on_submit(cid, now);
            if (act == RecoveryAction::RECOVER_QUERY_FIRST) {
                log_reject(in, "already in-flight — recover before resubmit");
                r.error = "recover-first"; return r;
            }
        }
        // 9. CASH RESERVATION — reserve for a BUY; resize/reject on insufficiency.
        bool reserved = false;
        if (ledger_ && in.is_buy && !in.is_exit) {
            double adj = ledger_->reserve_buy(cid, in.symbol, in.source ? in.source : "?", qty, in.ref_px);
            if (adj <= 0.0) {
                if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, false);
                log_reject(in, "insufficient cash"); r.error = "insufficient cash"; return r;
            }
            if (adj + 1e-12 < qty) {
                std::fprintf(stderr, "[GATEWAY] RESIZE src=%s %s qty %.8f -> %.8f (cash)\n",
                             in.source ? in.source : "?", in.symbol.c_str(), qty, adj);
            }
            qty = adj; reserved = true;
            if ((qty * in.ref_px) < min_notional_usd) {
                ledger_->release(cid);
                log_reject(in, "resized below MIN_NOTIONAL"); r.error = "below min notional"; return r;
            }
        } else if (ledger_ && !in.is_buy) {
            ledger_->note_sell(cid, in.symbol, in.source ? in.source : "?", qty, in.ref_px);
        }

        // 10. Forward to the (befriended) private executor with the deterministic id.
        r = ex_.execute(in.symbol, in.is_buy, qty, in.ref_px, cid);

        // 11. Apply the result to the truth ledger (via the stream path if wired).
        if (!r.ok) {
            if (reserved && ledger_) ledger_->release(cid);
            if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, false);
            return r;
        }
        if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, true);
        if (ledger_ || stream_) {
            ExecReport rep = build_report(r, in, qty, cid);
            if (stream_) stream_->feed_report(rep);      // stream drives the ledger (item 8 path)
            else         ledger_->apply_report(rep);
        }
        // Phase-4 item 22: additive parallel realistic-fill metric (observational).
        if (on_fill_observer) {
            double fq = r.executed_qty > 0.0 ? r.executed_qty : qty;
            on_fill_observer(in, fq, in.ref_px);
        }
        return r;
    }

private:
    static int64_t now_ms() {
        return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    ExecReport build_report(const OrderResult& r, const OrderIntent& in,
                            double submitted_qty, const std::string& cid) {
        ExecReport rep;
        rep.client_id = r.client_id.empty() ? cid : r.client_id;
        rep.symbol    = in.symbol;
        rep.source    = in.source ? in.source : "?";
        rep.is_buy    = in.is_buy;
        // Map the exchange status string to a distinct state.
        const std::string& s = r.status;
        if (r.shadow)                                   rep.state = OrderState::FILLED;
        else if (s == "FILLED")                         rep.state = OrderState::FILLED;
        else if (s == "PARTIALLY_FILLED" || s == "PARTIAL") rep.state = OrderState::PARTIAL;
        else if (s == "NEW" || s == "ACCEPTED")         rep.state = OrderState::ACCEPTED;
        else if (s == "CANCELED" || s == "CANCELLED")   rep.state = OrderState::CANCELLED;
        else if (s == "REJECTED" || s == "EXPIRED")     rep.state = OrderState::REJECTED;
        else                                            rep.state = OrderState::UNKNOWN;
        rep.filled_qty = r.executed_qty > 0.0 ? r.executed_qty
                       : (rep.state == OrderState::FILLED ? submitted_qty : 0.0);
        rep.avg_price  = r.avg_price > 0.0 ? r.avg_price : in.ref_px;
        rep.fee        = 0.0;   // ledger estimates from fee_rate when 0
        return rep;
    }
    void log_reject(const OrderIntent& in, const char* why) {
        std::fprintf(stderr, "[GATEWAY] REJECT src=%s %s %s qty=%.8f px=%.6f — %s\n",
                     in.source ? in.source : "?", in.is_buy ? "BUY" : "SELL",
                     in.symbol.c_str(), in.qty, in.ref_px, why);
    }
    Exec&           ex_;
    RuntimeMode     mode_;
    ExchangeLedger* ledger_  = nullptr;
    ExchangeFilters* filters_ = nullptr;
    ExchangeTimeSync* clock_ = nullptr;
    OrderIdRegistry* idreg_  = nullptr;
    UserDataStream*  stream_ = nullptr;
};

} // namespace chimera

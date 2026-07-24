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
#include <vector>
#include <algorithm>
#include <cctype>
#include <atomic>
#include <mutex>
#include <map>
#include <cmath>
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

    // ------------------------------------------------------------------------
    // LIVE PILOT SCOPE (2026-07-18, Binance go-live pilot). Bounds the FIRST
    // real-money window: in LIVE mode only, entries are restricted to an
    // explicit symbol allowlist, per-order notional is clamped, and aggregate
    // live gross exposure is capped. INERT in SHADOW/PAPER/DISABLED — shadow
    // research behaviour stays byte-identical. Exits are NEVER blocked (risk-
    // reducing; a non-pilot-symbol exit still passes so emergency flatten and
    // reconcile-driven closes always work). Configured from live_config.json
    // (live_pilot_symbols / live_pilot_max_order_usd / live_pilot_max_gross_usd)
    // in main.cpp; pilot_enabled=false + mode=LIVE means the caller opted into
    // FULL live (requires explicit live_full=true there — enforced at startup).
    // ------------------------------------------------------------------------
    bool                     pilot_enabled       = false;
    std::vector<std::string> pilot_symbols;               // UPPERCASE
    double                   pilot_max_order_usd = 0.0;   // per-entry clamp
    double                   pilot_max_gross_usd = 0.0;   // aggregate open cap
    double                   pilot_gross_usd     = 0.0;   // running (gateway thread)

    // ── ABSOLUTE PER-ORDER SIZE CEILING (2026-07-24, operator "size caps ... lowest
    //    possible unless changed explicitly by me"; twin of Omega's unconditional
    //    min-lot clamp). Fat-finger guard that CANNOT be disabled by config — applies
    //    in EVERY live mode (pilot OR full-live), unlike pilot_max_order_usd which is
    //    SKIPPED when pilot_enabled=false. A mis-sized order is physically impossible.
    //    Default $2000 = ~3-4× current live gross ($500-600): never a legit single
    //    order, always a fat-finger. Inert in SHADOW/PAPER. Exits never blocked.
    //    Raise ONLY on an explicit operator instruction.
    double                   hard_max_order_usd_ = 2000.0;

    // ── NATIVE BROKER-SIDE PROTECTIVE STOP (2026-07-24, operator: "protections
    //    must survive the bot dying"; crypto twin of Omega IbkrExecutionEngine's
    //    native STP-on-fill). On every opening LONG fill the gateway places a
    //    RESTING SELL STOP_LOSS_LIMIT AT BINANCE at fill*(1-pct). Binance holds it
    //    -> it fires autonomously even if the chimera process dies. Cancelled when
    //    the ledger shows the symbol flat (mirror of positionEnd cancel-on-flat) so
    //    a stale resting stop can't fire into a later re-buy. Re-armed on the next
    //    opening fill. LIVE + real-fill only: SHADOW/PAPER place no real order, so
    //    the shadow research record stays byte-identical. Long-only (spot).
    //    disaster_pct default 15% (matches Omega); tune per operator. Set
    //    native_stops_enabled_=false to disable entirely.
    bool                     native_stops_enabled_     = true;
    double                   native_disaster_stop_pct_ = 15.0;   // % below fill
    // Protective-stop hooks (null => stop management inert — a bare mock executor
    // needs neither method, so existing gateway unit tests compile unchanged).
    // Wired in main.cpp to SpotExecutor::place_protective_stop / cancel_protective_stop.
    //   place: (symbol, held_qty, entry_px, stop_pct) -> OrderResult (cid/oid identify the stop)
    //   cancel:(symbol, cid, oid) -> bool
    std::function<OrderResult(const std::string&, double, double, double)> place_stop_fn;
    std::function<bool(const std::string&, const std::string&, long)>      cancel_stop_fn;

    OrderResult submit(const OrderIntent& in) {
        OrderResult r;
        // 0. CIRCUIT-BREAKER (sticky) — once an order-storm tripped the breaker,
        //    every subsequent send (entry OR exit) is hard-rejected until a restart.
        //    Already logged once at trip time. See the breaker block just before the
        //    executor call (step 10) for the trip conditions.
        if (circuit_tripped_.load()) { r.error = "circuit-breaker tripped"; return r; }
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
        // LOT_SIZE QUARANTINE — a leg that clustered -1013 rejects is disabled for
        // ENTRIES (exits always pass — risk-reducing). Targeted per-leg, not a book halt.
        if (!in.is_exit) {
            int qn; { std::lock_guard<std::mutex> lk(cb_mtx_); qn = lotsize_rejects_[in.symbol]; }
            if (qn >= LOTSIZE_QUARANTINE) {
                log_reject(in, "LOT_SIZE quarantine (leg disabled)");
                r.error = "LOT_SIZE quarantine"; return r;
            }
        }
        // 4. EXCHANGE FILTERS — normalize qty before approval.
        double qty = in.qty;
        if (filters_) {
            NormalizedOrder n = filters_->normalize(in.symbol, qty, in.ref_px, /*market*/true);
            if (!n.ok) {
                std::string why = "filter: " + n.reason;
                log_reject(in, why.c_str()); r.error = why; return r;
            }
            // S-2026-07-23: NEVER send RAW qty to Binance (guaranteed -1013). The
            // S-20 code merely WARNED and let the unfloored qty through -> the
            // silent-fallback -1013 class the operator paused the book for. With the
            // startup per-symbol LOT_SIZE backfill (main.cpp) every live symbol has a
            // valid filter, so this is unreachable for a real live symbol; if it DOES
            // fire, the symbol has no cached step -> REJECT honestly (skip the doomed
            // order) rather than post it. Exits are never blocked (risk-reducing) and
            // sells still floor via normalize() above.
            if (in.is_buy && !in.is_exit && !filters_->has_valid(in.symbol)) {
                std::printf("[FILTERS] REJECT live BUY %s — no valid LOT_SIZE cached "
                            "(backfill gap); skipping doomed order (was: raw qty %.8f -> -1013)\n",
                            in.symbol.c_str(), qty);
                std::fflush(stdout);
                log_reject(in, "no valid LOT_SIZE (backfill gap)");
                r.error = "no valid LOT_SIZE"; return r;
            }
            qty = n.qty;
        }
        // 4a. ABSOLUTE SIZE CEILING — config-independent fat-finger guard. Applies in
        //     EVERY live mode (pilot OR full-live); the pilot clamp below is skipped
        //     when pilot_enabled=false, so this is the ONLY per-order cap in full-live.
        //     Inert in SHADOW/PAPER. Exits never blocked.
        if (mode_ == RuntimeMode::LIVE && !in.is_exit &&
            hard_max_order_usd_ > 0.0 && qty * in.ref_px > hard_max_order_usd_) {
            double nq = hard_max_order_usd_ / in.ref_px;
            std::fprintf(stderr,
                "[GATEWAY] HARD SIZE-CAP src=%s %s qty %.8f -> %.8f (absolute order ceiling $%.2f)\n",
                in.source ? in.source : "?", in.symbol.c_str(), qty, nq, hard_max_order_usd_);
            qty = nq;
        }
        // 4b. LIVE PILOT SCOPE — entries only; inert outside LIVE mode.
        //     Symbol must be allowlisted; per-order notional clamped; aggregate
        //     live gross capped. Exits never touch this block.
        if (pilot_enabled && mode_ == RuntimeMode::LIVE && !in.is_exit) {
            std::string su = in.symbol;
            for (auto& c : su) c = (char)std::toupper((unsigned char)c);
            if (std::find(pilot_symbols.begin(), pilot_symbols.end(), su)
                    == pilot_symbols.end()) {
                log_reject(in, "pilot-scope: symbol not in live pilot allowlist");
                r.error = "pilot-scope symbol"; return r;
            }
            if (pilot_max_order_usd > 0.0 && qty * in.ref_px > pilot_max_order_usd) {
                double nq = pilot_max_order_usd / in.ref_px;
                std::fprintf(stderr,
                    "[GATEWAY] PILOT RESIZE src=%s %s qty %.8f -> %.8f (order cap $%.2f)\n",
                    in.source ? in.source : "?", su.c_str(), qty, nq, pilot_max_order_usd);
                qty = nq;
            }
            if (pilot_max_gross_usd > 0.0 &&
                pilot_gross_usd + qty * in.ref_px > pilot_max_gross_usd + 1e-9) {
                log_reject(in, "pilot-scope: aggregate live gross cap");
                r.error = "pilot-scope gross cap"; return r;
            }
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

        // 9b. ORDER CIRCUIT-BREAKER (2026-07-24) — the LAST gate before a real POST
        //     to Binance. Makes an order-storm PHYSICALLY IMPOSSIBLE. Root incident
        //     (Omega, mirrored here per operator demand): ConnorsRSI2 fired 24,776
        //     unfilled orders in ~40s the instant its exec reconnected because there
        //     was NO cap between the connect-check and placeOrder. The crypto path had
        //     the same gap: a runaway engine could spam Binance without limit
        //     (-1003 order-rate ban + exposure). Two hard limits + a STICKY halt:
        //       (a) global > MAX_ORDERS_PER_SEC in any rolling 1s window, and
        //       (b) per-symbol >= MAX_UNFILLED_PER_SYM sends with 0 fills (the runaway
        //           signature — a filling trade resets its symbol's counter below).
        //     LIVE-only: in SHADOW/PAPER no order reaches Binance (no -1003 risk) and
        //     research books legitimately batch full-universe rebalances, so scoping
        //     to LIVE keeps the shadow record byte-identical and avoids false trips.
        //     On trip: circuit_tripped_ (sticky) -> every submit() returns an error;
        //     cleared ONLY by a restart (operator investigates the runaway first).
        //     Emergency-flatten uses a SEPARATE path (SpotExecutor::emergency_flatten),
        //     so a manual flatten still works after a trip.
        if (mode_ == RuntimeMode::LIVE) {
            std::lock_guard<std::mutex> lk(cb_mtx_);
            long long now_cb = (long long)now_ms();
            if (now_cb - rate_win_start_ms_ >= 1000) { rate_win_start_ms_ = now_cb; rate_win_count_ = 0; }
            if (++rate_win_count_ > MAX_ORDERS_PER_SEC) {
                trip_circuit_("global rate > " + std::to_string(MAX_ORDERS_PER_SEC) + " orders/sec");
                if (reserved && ledger_) ledger_->release(cid);
                if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, false);
                r.error = "circuit-breaker: rate"; return r;
            }
            // ENTRIES only (2026-07-24b): an unfilled EXIT is not a runaway-entry signature,
            // and tripping on it would BLOCK exits (only emergency_flatten survives). Exits
            // are risk-reducing — they must never trip or be blocked by this cap.
            if (!in.is_exit && ++unfilled_by_sym_[in.symbol] > MAX_UNFILLED_PER_SYM) {
                trip_circuit_(in.symbol + ": " + std::to_string(unfilled_by_sym_[in.symbol]) +
                              " orders sent, 0 fills (runaway loop)");
                if (reserved && ledger_) ledger_->release(cid);
                if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, false);
                r.error = "circuit-breaker: per-symbol unfilled"; return r;
            }
        }

        // 10. Forward to the (befriended) private executor with the deterministic id.
        r = ex_.execute(in.symbol, in.is_buy, qty, in.ref_px, cid);

        // CIRCUIT-BREAKER: a real fill clears this symbol's unfilled counter so
        // normal filling trades never approach the per-symbol cap.
        if (mode_ == RuntimeMode::LIVE && r.ok && r.executed_qty > 0.0) {
            std::lock_guard<std::mutex> lk(cb_mtx_);
            unfilled_by_sym_[in.symbol] = 0;
        }

        // 11. Apply the result to the truth ledger (via the stream path if wired).
        if (!r.ok) {
            if (reserved && ledger_) ledger_->release(cid);
            if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, false);
            // ── LOT_SIZE / -1013 CONSEQUENCE (2026-07-24): a reject was silently
            //    swallowed here. Make it LOUD, and quarantine the leg on a cluster so
            //    a broken-precision leg stops silently bouncing every cycle.
            if (r.error.find("LOT_SIZE") != std::string::npos ||
                r.error.find("-1013")    != std::string::npos) {
                int n;
                { std::lock_guard<std::mutex> lk(cb_mtx_); n = ++lotsize_rejects_[in.symbol]; }
                std::fprintf(stderr,
                    "[GATEWAY] LOT_SIZE REJECT src=%s %s qty=%.8f (#%d) -- Binance -1013; "
                    "leg NOT trading. %s\n",
                    in.source ? in.source : "?", in.symbol.c_str(), qty, n,
                    n >= LOTSIZE_QUARANTINE
                        ? "*** QUARANTINED *** (entries blocked until restart + qty-floor fix)"
                        : "fix the qty floor/precision mapping.");
                std::fflush(stderr);
            }
            return r;
        }
        if (idreg_ && in.signal_id != 0) idreg_->on_result(cid, true);
        // Pilot gross tracking — fills only, LIVE only. BUY adds, SELL reduces
        // (floored at 0). Runs on the gateway/WS thread like the rest of submit().
        if (pilot_enabled && mode_ == RuntimeMode::LIVE) {
            double fq  = r.executed_qty > 0.0 ? r.executed_qty : qty;
            double fpx = r.avg_price    > 0.0 ? r.avg_price    : in.ref_px;
            if (in.is_buy) pilot_gross_usd += fq * fpx;
            else { pilot_gross_usd -= fq * fpx; if (pilot_gross_usd < 0.0) pilot_gross_usd = 0.0; }
        }
        if (ledger_ || stream_) {
            ExecReport rep = build_report(r, in, qty, cid);
            if (stream_) stream_->feed_report(rep);      // stream drives the ledger (item 8 path)
            else         ledger_->apply_report(rep);
        }
        // ── NATIVE BROKER-SIDE PROTECTIVE STOP (2026-07-24) — arm-on-fill /
        //    cancel-on-flat, mirror of Omega IbkrExecutionEngine. LIVE + real fill
        //    only (shadow places nothing). The ledger was just driven by the report
        //    above (the stream handler is synchronous), so position()/avg_price()
        //    reflect this fill. Long-only spot: a BUY that leaves a held position
        //    arms a resting SELL stop; a SELL that flattens cancels it.
        if (native_stops_enabled_ && place_stop_fn && mode_ == RuntimeMode::LIVE
            && !r.shadow && ledger_) {
            manage_protective_stop_(in.symbol);
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

    // ── PROTECTIVE-STOP MANAGEMENT (2026-07-24) ------------------------------
    //    Ensure the broker-side resting stop matches the ledger truth for ONE
    //    symbol: arm one if a position is held and none rests; cancel + re-arm to
    //    the current held qty if the size changed materially; cancel if flat.
    //    Called on every LIVE fill and from reconcile_stops(). Uses avg_price as
    //    the stop anchor (Omega uses broker avg entry — same idea). "Flat" =
    //    held notional below MIN_NOTIONAL (Binance can't rest a sub-min stop, and
    //    dust is not worth protecting).
    struct RestingStop { std::string cid; long oid = 0; double qty = 0.0; };
    std::map<std::string, RestingStop> resting_stop_;   // symbol -> resting stop
    std::mutex                         stop_mtx_;
    void manage_protective_stop_(const std::string& symbol) {
        double held  = ledger_->position(symbol);
        double entry = ledger_->avg_price(symbol);
        double px    = entry > 0.0 ? entry : 0.0;
        bool   flat  = (held <= 0.0) || (px > 0.0 && held * px < min_notional_usd);

        std::lock_guard<std::mutex> lk(stop_mtx_);
        auto it = resting_stop_.find(symbol);
        bool have = (it != resting_stop_.end());

        if (flat) {
            if (have) {
                if (cancel_stop_fn) cancel_stop_fn(symbol, it->second.cid, it->second.oid);
                resting_stop_.erase(it);
            }
            return;
        }
        // Held position. Arm if none, or re-arm if the protected qty drifted from
        // the held qty by > ~1% (a partial exit / add changed the size).
        if (have) {
            double q = it->second.qty;
            if (q > 0.0 && std::fabs(held - q) / q <= 0.01) return;  // still matches
            if (cancel_stop_fn) cancel_stop_fn(symbol, it->second.cid, it->second.oid);
            resting_stop_.erase(it);
        }
        if (px <= 0.0 || !place_stop_fn) return;   // no entry anchor / hook -> skip
        OrderResult sr = place_stop_fn(symbol, held, px, native_disaster_stop_pct_);
        if (sr.ok) resting_stop_[symbol] = RestingStop{ sr.client_id, sr.order_id, held };
    }

public:
    // ── reconcile_stops — mirror of Omega positionEnd: given the ledger truth,
    //    ensure EVERY held position has a broker-side stop and no flat symbol has
    //    a stale one. Call at boot (after the startup reconcile) and periodically
    //    from the live reconcile loop so positions that filled before this path
    //    existed — or lost their stop — get re-protected. Safe no-op in shadow /
    //    when disabled. `symbols` is the set the ledger may hold (e.g. the live
    //    pilot/traded universe); pass every symbol you want checked.
    void reconcile_stops(const std::vector<std::string>& symbols) {
        if (!native_stops_enabled_ || !place_stop_fn || mode_ != RuntimeMode::LIVE || !ledger_) return;
        for (const auto& s : symbols) manage_protective_stop_(s);
        // Cancel stops for symbols no longer in the ledger's held set.
        std::vector<std::string> stale;
        {
            std::lock_guard<std::mutex> lk(stop_mtx_);
            for (auto& kv : resting_stop_)
                if (ledger_->position(kv.first) <= 0.0) stale.push_back(kv.first);
        }
        for (const auto& s : stale) manage_protective_stop_(s);
    }

    // No-arg overload — derive the live universe from ledger truth (every held
    // position) so a caller that does not have the symbol list in scope (e.g. the
    // periodic live loop, whose ledger is block-scoped elsewhere) can still drive
    // it via the gateway's own ledger_. Same LIVE-only self-gate as above; inert in
    // shadow/paper and when disabled. Held-set + the internal stale sweep together
    // arm every held symbol and cancel every flat one.
    void reconcile_stops() {
        if (!native_stops_enabled_ || !place_stop_fn || mode_ != RuntimeMode::LIVE || !ledger_) return;
        reconcile_stops(ledger_->held_symbols());
    }
private:

    // ── ORDER CIRCUIT-BREAKER (2026-07-24) — mirror of Omega's IbkrExecutionEngine
    //    place_order breaker. Thresholds identical to Omega (25/sec, 8 unfilled/sym):
    //    each LIVE execute() blocks on a real HTTP POST (~50-200ms round trip), so
    //    legitimate LIVE orders — including a serial full-universe rebalance — are
    //    naturally throttled well under 25/sec; a fast-reject runaway loop is not.
    //    The per-symbol-unfilled cap is the precise single-symbol runaway detector
    //    (the incident was one symbol); the global rate is the multi-symbol backstop.
    static constexpr int          MAX_ORDERS_PER_SEC   = 25;
    static constexpr int          MAX_UNFILLED_PER_SYM = 8;
    std::atomic<bool>             circuit_tripped_{false};
    std::mutex                    cb_mtx_;                 // guards the three below
    long long                     rate_win_start_ms_ = 0;
    int                           rate_win_count_    = 0;
    std::map<std::string,int>     unfilled_by_sym_;
    // ── LOT_SIZE / -1013 QUARANTINE (2026-07-24): a Binance -1013 LOT_SIZE reject
    //    that comes back UNDER the 8-count unfilled trip was SILENTLY skipped — the
    //    leg just stopped trading, no alert, no consequence (BTC-ROC/SOL-EMAX went
    //    dead this way). Now: every LOT_SIZE reject is LOUD, and after LOTSIZE_QUARANTINE
    //    on one sym that leg is QUARANTINED (entries blocked) — a TARGETED per-leg
    //    disable, NOT a whole-book halt. Cleared on restart (fix the qty floor first).
    static constexpr int          LOTSIZE_QUARANTINE = 3;
    std::map<std::string,int>     lotsize_rejects_;        // guarded by cb_mtx_

    void trip_circuit_(const std::string& why) {           // call under cb_mtx_
        if (circuit_tripped_.exchange(true)) return;       // one-shot
        std::fprintf(stderr,
            "[GATEWAY] *** CIRCUIT-BREAKER TRIPPED *** %s -- ALL ORDERS HALTED "
            "(exec hard-disabled; restart + investigate the runaway before re-enabling)\n",
            why.c_str());
        std::fflush(stderr);
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

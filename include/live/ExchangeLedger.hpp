#pragma once
// ============================================================================
// ExchangeLedger — THE single source of truth for position / avg-price / fills /
// pending-orders / cash. (Phase-2 review fix, 2026-07-11.)
//
// BEFORE: every sleeve kept its OWN fictional holdings map — XSec's per-callback
// `static std::map hold`, RipRider's `rip_nav/exitPrice` recompute, EdgeEngine's
// notional sizing — none reconciled to a real fill and all sized off the SAME
// max_position_usd, so the book could be overbooked many times over and an exit
// could sell a quantity the exchange never confirmed.
//
// AFTER: strategies READ this ledger and keep only attribution records. Position
// and cash mutate ONLY from execution reports (apply_report) — never from a
// strategy's optimistic intent. Cash is reserved BEFORE an order is submitted so
// three sleeves can no longer each book the full capital.
//
// SHADOW: the shadow executor returns an immediate FILLED OrderResult; the
// gateway turns that into an ExecReport and drives this ledger from it, so the
// exact same code path that runs live is exercised in shadow.
//
// Header-only, no curl/REST dependency, so it is cheaply unit-testable.
// ============================================================================
#include <string>
#include <map>
#include <cmath>
#include <cstdint>
#include <vector>

namespace chimera {

// Order lifecycle states modelled distinctly (review item 3).
enum class OrderState { ACCEPTED, PARTIAL, FILLED, CANCELLED, REJECTED, UNKNOWN };

inline const char* order_state_str(OrderState s) {
    switch (s) {
        case OrderState::ACCEPTED:  return "ACCEPTED";
        case OrderState::PARTIAL:   return "PARTIAL";
        case OrderState::FILLED:    return "FILLED";
        case OrderState::CANCELLED: return "CANCELLED";
        case OrderState::REJECTED:  return "REJECTED";
        case OrderState::UNKNOWN:   return "UNKNOWN";
    }
    return "?";
}

// A normalized execution report — the ONLY thing allowed to mutate holdings/cash.
// filled_qty / avg_price describe the fill carried by THIS report (incremental for
// a PARTIAL stream; total for a single FILLED). fee is in quote asset (USDT).
struct ExecReport {
    std::string client_id;
    std::string symbol;       // upper, e.g. "BTCUSDT"
    std::string source = "?"; // attribution tag (engine / sleeve)
    bool        is_buy  = true;
    OrderState  state   = OrderState::UNKNOWN;
    double      filled_qty = 0.0;   // base qty filled by THIS report
    double      avg_price  = 0.0;   // fill price
    double      fee        = 0.0;   // quote-asset fee
};

struct SymbolPos {
    double qty       = 0.0;   // base held
    double avg_price = 0.0;   // vwap of the open position
    double fees_paid = 0.0;   // cumulative quote fees on this symbol
    int    fills     = 0;
};

class ExchangeLedger {
public:
    // --- configuration -------------------------------------------------------
    // total_cash>0 with enforce=true => cash reservation REJECTS/RESIZES overbook.
    // total_cash<=0 => track-only (unlimited): reservation never blocks, still
    //                  tracks position/attribution. Used in shadow to preserve the
    //                  existing research record unless the operator opts in.
    void configure(double total_cash, bool enforce, double fee_rate = 0.001) {
        total_cash_  = total_cash;
        enforce_cash_ = enforce && total_cash > 0.0;
        fee_rate_    = fee_rate;
        reserved_cash_ = 0.0;
    }
    void set_cash(double c)      { total_cash_ = c; }
    double fee_rate() const      { return fee_rate_; }
    bool enforce_cash() const    { return enforce_cash_; }

    // --- cash view (review item 2) ------------------------------------------
    double total_cash()    const { return total_cash_; }
    double reserved_cash() const { return reserved_cash_; }
    double available_cash() const { return total_cash_ - reserved_cash_; }
    // Expected proceeds if every held base unit were sold at a reference price map.
    double pending_buys() const {
        double s = 0.0; for (auto& kv : pending_) if (kv.second.is_buy) s += kv.second.reserved; return s;
    }

    // --- reservation (called by the gateway BEFORE submission) --------------
    // Reserves cash for a BUY. Returns the (possibly resized) qty to submit; 0.0
    // means REJECT (cannot afford even a resized order). Exits never reserve.
    // Records the reservation under client_id so apply_report can release it.
    double reserve_buy(const std::string& client_id, const std::string& symbol,
                       const std::string& source, double qty, double px) {
        if (qty <= 0.0 || px <= 0.0) return 0.0;
        double cost = qty * px * (1.0 + fee_rate_);
        double adj_qty = qty;
        if (enforce_cash_) {
            double avail = available_cash();
            if (avail <= 0.0) return 0.0;                 // fully booked -> reject
            if (cost > avail) {
                adj_qty = (avail / (px * (1.0 + fee_rate_)));  // resize down to affordable
                cost = adj_qty * px * (1.0 + fee_rate_);
                if (adj_qty <= 0.0) return 0.0;
            }
        }
        Pending p; p.symbol = symbol; p.source = source; p.is_buy = true;
        p.qty = adj_qty; p.px = px; p.reserved = cost; p.state = OrderState::ACCEPTED;
        pending_[client_id] = p;
        reserved_cash_ += cost;
        return adj_qty;
    }

    // Records a pending SELL (no cash reserved; frees base attribution on fill).
    void note_sell(const std::string& client_id, const std::string& symbol,
                   const std::string& source, double qty, double px) {
        Pending p; p.symbol = symbol; p.source = source; p.is_buy = false;
        p.qty = qty; p.px = px; p.reserved = 0.0; p.state = OrderState::ACCEPTED;
        pending_[client_id] = p;
    }

    // Release an outstanding reservation (order failed to submit / rejected pre-fill).
    void release(const std::string& client_id) {
        auto it = pending_.find(client_id);
        if (it == pending_.end()) return;
        if (it->second.is_buy) reserved_cash_ -= it->second.reserved;
        if (reserved_cash_ < 0.0) reserved_cash_ = 0.0;
        pending_.erase(it);
    }

    // --- the ONLY mutation of position/cash (review item 3) -----------------
    void apply_report(const ExecReport& r) {
        auto pit = pending_.find(r.client_id);
        Pending* pend = (pit != pending_.end()) ? &pit->second : nullptr;

        switch (r.state) {
            case OrderState::REJECTED:
            case OrderState::CANCELLED:
                // No holdings change; free any reservation and drop the pending.
                if (pend && pend->is_buy) { reserved_cash_ -= pend->reserved; if (reserved_cash_ < 0) reserved_cash_ = 0; }
                if (pit != pending_.end()) pending_.erase(pit);
                return;

            case OrderState::UNKNOWN:
                // Ambiguous — do NOT touch holdings; leave the reservation intact so
                // recovery (query-by-id) can resolve it. Mark for the reconciler.
                if (pend) pend->state = OrderState::UNKNOWN;
                return;

            case OrderState::ACCEPTED:
                // Working order, no fill yet. Reservation already held for buys.
                if (pend) pend->state = OrderState::ACCEPTED;
                return;

            case OrderState::PARTIAL:
            case OrderState::FILLED:
                break;
        }

        double q = r.filled_qty, px = r.avg_price;
        if (q <= 0.0 || px <= 0.0) { // a FILLED with no qty — treat as no-op fill
            if (r.state == OrderState::FILLED && pit != pending_.end()) {
                if (pend && pend->is_buy) { reserved_cash_ -= pend->reserved; if (reserved_cash_ < 0) reserved_cash_ = 0; }
                pending_.erase(pit);
            }
            return;
        }

        SymbolPos& sp = pos_[r.symbol];
        const std::string akey = attrib_key(r.source, r.symbol);
        double fee = r.fee > 0.0 ? r.fee : q * px * fee_rate_;

        if (r.is_buy) {
            double new_qty = sp.qty + q;
            sp.avg_price = new_qty > 0.0 ? (sp.qty * sp.avg_price + q * px) / new_qty : 0.0;
            sp.qty = new_qty;
            sp.fees_paid += fee; sp.fills++;
            attrib_[akey] += q;
            // Cash: spend actual; unwind the reservation this fill consumed.
            total_cash_ -= (q * px + fee);
            if (pend) {
                double consume = q * px * (1.0 + fee_rate_);
                pend->reserved -= consume; reserved_cash_ -= consume;
                if (pend->reserved < 0) pend->reserved = 0;
                if (reserved_cash_ < 0) reserved_cash_ = 0;
            }
        } else {
            double sell = q > sp.qty ? sp.qty : q;   // never oversell the confirmed position
            sp.qty -= sell;
            sp.fees_paid += fee; sp.fills++;
            if (sp.qty <= 1e-12) { sp.qty = 0.0; sp.avg_price = 0.0; }
            double& a = attrib_[akey];
            a -= sell; if (a < 0.0) a = 0.0;
            total_cash_ += (sell * px - fee);
        }

        // FILLED completes the order; PARTIAL leaves it working (reservation remains
        // for the unfilled buy remainder).
        if (r.state == OrderState::FILLED && pit != pending_.end()) {
            if (pend && pend->is_buy && pend->reserved > 0.0) {
                reserved_cash_ -= pend->reserved; if (reserved_cash_ < 0) reserved_cash_ = 0;
            }
            pending_.erase(pit);
        } else if (pend) {
            pend->state = OrderState::PARTIAL;
        }
    }

    // --- reads (strategies READ, they do not own quantity) ------------------
    double position(const std::string& symbol) const {
        auto it = pos_.find(symbol); return it == pos_.end() ? 0.0 : it->second.qty;
    }
    double avg_price(const std::string& symbol) const {
        auto it = pos_.find(symbol); return it == pos_.end() ? 0.0 : it->second.avg_price;
    }
    double fees_paid(const std::string& symbol) const {
        auto it = pos_.find(symbol); return it == pos_.end() ? 0.0 : it->second.fees_paid;
    }
    // Attributed base qty a given sleeve holds of a symbol — the exact quantity an
    // exit path must close (review item 4). Never oversells the real position.
    double attributed_qty(const std::string& source, const std::string& symbol) const {
        auto it = attrib_.find(attrib_key(source, symbol));
        double a = it == attrib_.end() ? 0.0 : it->second;
        double p = position(symbol);
        return a > p ? p : a;   // clamp to the exchange-confirmed position
    }
    // Phase-3 (item 15): per-symbol reads the SpotPortfolioAllocator nets against.
    // Mark-to-ref value of the held position, and the notional of any pending BUY
    // for this symbol (an in-flight buy the allocator must not double-order).
    double position_value(const std::string& symbol, double ref_px) const {
        return position(symbol) * ref_px;
    }
    double pending_buy_value(const std::string& symbol) const {
        double v = 0.0;
        for (auto& kv : pending_)
            if (kv.second.is_buy && kv.second.symbol == symbol)
                v += kv.second.qty * kv.second.px;
        return v;
    }

    bool has_pending(const std::string& client_id) const { return pending_.count(client_id) != 0; }
    OrderState pending_state(const std::string& client_id) const {
        auto it = pending_.find(client_id); return it == pending_.end() ? OrderState::UNKNOWN : it->second.state;
    }
    // Inject a known-working order at startup reconciliation (so no duplicate is sent).
    void adopt_pending(const std::string& client_id, const std::string& symbol,
                       const std::string& source, bool is_buy, double qty, double px) {
        Pending p; p.symbol = symbol; p.source = source; p.is_buy = is_buy;
        p.qty = qty; p.px = px; p.reserved = is_buy ? qty * px * (1.0 + fee_rate_) : 0.0;
        p.state = OrderState::ACCEPTED;
        pending_[client_id] = p;
        if (is_buy) reserved_cash_ += p.reserved;
    }
    // Seed a KNOWN pre-boot holding at startup reconciliation (2026-07-24, native-
    // stop residual). CASH-NEUTRAL by design: unlike apply_report(buy) this does NOT
    // touch total_cash_ / reservations, because the coins were acquired BEFORE this
    // process started, so their cost already left the account — booking a buy here
    // would double-charge seed cash. It exists purely so position()/avg_price()/
    // held_symbols() report a pre-existing balance, letting StartupReconciler agree
    // with the exchange and ExecutionGateway::reconcile_stops() arm a native
    // protective stop on it. avg_price is used ONLY as that stop's anchor. Idempotent
    // (SET, not +=) so a re-seed cannot drift the qty. Note (honest): for an
    // underwater pre-boot hold the caller clamps avg_price to market, so unrealized
    // PnL on such a seed reads ~0 until it trades — protection is the goal here.
    void seed_position(const std::string& symbol, double qty, double avg_price) {
        if (qty <= 0.0) return;
        SymbolPos& sp = pos_[symbol];
        sp.qty       = qty;
        sp.avg_price = avg_price > 0.0 ? avg_price : sp.avg_price;
    }

    size_t num_pending() const { return pending_.size(); }
    size_t num_positions() const { size_t n = 0; for (auto& kv : pos_) if (kv.second.qty > 0) ++n; return n; }
    // Symbols with a live (qty>0) held position — the exact set that needs a
    // broker-side protective stop. Used by ExecutionGateway::reconcile_stops()
    // so the live universe is derived from ledger truth, not a separate list.
    std::vector<std::string> held_symbols() const {
        std::vector<std::string> out;
        for (auto& kv : pos_) if (kv.second.qty > 0) out.push_back(kv.first);
        return out;
    }

    // Total book value at a reference price map (symbol->px). Cash + mark-to-ref.
    double equity(const std::map<std::string, double>& ref_px) const {
        double e = total_cash_;
        for (auto& kv : pos_) {
            auto it = ref_px.find(kv.first);
            e += kv.second.qty * (it != ref_px.end() ? it->second : kv.second.avg_price);
        }
        return e;
    }

private:
    static std::string attrib_key(const std::string& src, const std::string& sym) {
        return src + "\x1f" + sym;
    }
    struct Pending {
        std::string symbol, source; bool is_buy = true;
        double qty = 0, px = 0, reserved = 0; OrderState state = OrderState::ACCEPTED;
    };

    double total_cash_    = 0.0;
    double reserved_cash_ = 0.0;
    bool   enforce_cash_  = false;
    double fee_rate_      = 0.001;
    std::map<std::string, SymbolPos> pos_;
    std::map<std::string, double>    attrib_;
    std::map<std::string, Pending>   pending_;
};

} // namespace chimera

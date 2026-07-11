#pragma once
// ============================================================================
// RealisticFill — Phase-4 review fix, item 22 (SHADOW-FILL REALISM).
//
// Today's shadow fills execute at the SIGNAL price (SpotExecutor.hpp:152,
// `r.avg_price = price`) — optimistic: zero spread, zero slippage, always a
// full instant fill. This UNDERSTATES cost, especially for the fast UpJump
// legs that cross a wide book.
//
// *** CRITICAL — ADDITIVE ONLY. *** This module NEVER touches the running
// signal-price shadow book or the 32-cell UpJump threshold GRID. It maintains
// a PARALLEL "realistic-fill" metric alongside them so the operator can COMPARE
// the two books. The signal-price book stays the default of record for
// continuity; the realistic book models spread + slippage + taker fee + queue
// partial-fill so realistic_pnl <= signal_pnl by construction (costs only sub-
// tract). It is a measurement layer, not an execution change.
//
// Header-only, dependency-free, unit-tested standalone.
// ============================================================================
#include <string>
#include <map>
#include <cmath>
#include <cstdio>

namespace chimera {

// Cost model. Defaults are conservative Binance-spot-like starting points;
// the operator tunes per-symbol from real book snapshots before any go-live.
struct FillModelParams {
    double half_spread_bp   = 2.0;   // half the typical bid/ask spread (bp of px)
    double slippage_bp      = 1.0;   // market-impact / walk-the-book slippage (bp)
    double taker_fee_bp     = 10.0;  // Binance spot taker = 0.10% = 10bp
    double queue_fill_ratio = 1.0;   // fraction of requested qty that fills (partial/queue)
};

struct RealisticFill {
    double fill_px    = 0.0;   // realistic fill price (worse than signal)
    double filled_qty = 0.0;   // qty actually filled (<= requested if partial)
    double fee_quote  = 0.0;   // quote-asset (USDT) fee on the filled notional
    double cost_bp    = 0.0;   // total adverse cost vs signal, in bp of notional
};

class RealisticFillModel {
public:
    void configure(const FillModelParams& p) { p_ = p; }
    const FillModelParams& params() const { return p_; }

    // Apply the cost model to a signal-price order. A BUY pays spread+slippage
    // UP (worse), a SELL receives them DOWN (worse). Fee applies both sides.
    RealisticFill apply(bool is_buy, double signal_px, double qty) const {
        RealisticFill r;
        if (signal_px <= 0.0 || qty <= 0.0) return r;
        double adj = (p_.half_spread_bp + p_.slippage_bp) / 1e4;
        r.fill_px    = is_buy ? signal_px * (1.0 + adj) : signal_px * (1.0 - adj);
        double qr    = p_.queue_fill_ratio;
        if (qr < 0.0) qr = 0.0; if (qr > 1.0) qr = 1.0;
        r.filled_qty = qty * qr;
        double notional = r.filled_qty * r.fill_px;
        r.fee_quote  = notional * (p_.taker_fee_bp / 1e4);
        // Total adverse cost vs a signal-price fill (spread+slippage on the
        // filled leg + fee), expressed in bp of the REQUESTED notional.
        double req_notional = qty * signal_px;
        double px_cost = std::fabs(r.fill_px - signal_px) * r.filled_qty;
        r.cost_bp = req_notional > 0.0 ? (px_cost + r.fee_quote) / req_notional * 1e4 : 0.0;
        return r;
    }

private:
    FillModelParams p_;
};

// ----------------------------------------------------------------------------
// ShadowFillComparator — parallel two-book realized-PnL comparator. Feeds every
// fill into BOTH a signal-price book (optimistic, = current record) and a
// realistic book (costs modelled). Round-trip realized PnL per book. Never
// writes to the real shadow ledger or the grid — a pure side metric.
// ----------------------------------------------------------------------------
class ShadowFillComparator {
public:
    void configure(const FillModelParams& p) { model_.configure(p); }

    // Record a fill. BUY opens/adds; SELL closes (realizes vs avg cost). Both
    // books tracked independently under the same key (tag|symbol).
    void on_fill(const std::string& tag, const std::string& symbol,
                 bool is_buy, double signal_px, double qty) {
        if (signal_px <= 0.0 || qty <= 0.0) return;
        std::string key = tag + "|" + symbol;
        Book& b = books_[key];
        RealisticFill rf = model_.apply(is_buy, signal_px, qty);
        if (is_buy) {
            // signal book: fill at signal_px, no fee
            double nq = b.s_qty + qty;
            b.s_avg = nq > 0.0 ? (b.s_qty * b.s_avg + qty * signal_px) / nq : 0.0;
            b.s_qty = nq;
            // realistic book: fill at rf.fill_px on rf.filled_qty; fee realized now
            double rq = b.r_qty + rf.filled_qty;
            b.r_avg = rq > 0.0 ? (b.r_qty * b.r_avg + rf.filled_qty * rf.fill_px) / rq : 0.0;
            b.r_qty = rq;
            b.r_fees += rf.fee_quote;
        } else {
            // SELL closes up to held qty on each book independently.
            double sq = std::min(qty, b.s_qty);
            b.s_realized += sq * (signal_px - b.s_avg);
            b.s_qty -= sq;
            double rq = std::min(rf.filled_qty, b.r_qty);
            b.r_realized += rq * (rf.fill_px - b.r_avg) - rf.fee_quote;
            b.r_qty -= rq;
            b.r_fees += rf.fee_quote;
        }
    }

    double signal_book_pnl() const {
        double s = 0.0; for (auto& kv : books_) s += kv.second.s_realized; return s;
    }
    double realistic_book_pnl() const {
        double s = 0.0; for (auto& kv : books_) s += kv.second.r_realized; return s;
    }
    double total_modelled_cost() const {
        // The gap the operator compares: signal book minus realistic book.
        return signal_book_pnl() - realistic_book_pnl();
    }
    size_t books() const { return books_.size(); }

    void print_summary(const char* prefix = "[FILL-REALISM]") const {
        std::printf("%s signal_book_pnl=%.2f realistic_book_pnl=%.2f modelled_cost=%.2f (%zu books)\n",
                    prefix, signal_book_pnl(), realistic_book_pnl(),
                    total_modelled_cost(), books_.size());
        std::fflush(stdout);
    }

private:
    struct Book {
        double s_qty = 0.0, s_avg = 0.0, s_realized = 0.0;           // signal-price book
        double r_qty = 0.0, r_avg = 0.0, r_realized = 0.0, r_fees = 0.0; // realistic book
    };
    RealisticFillModel model_;
    std::map<std::string, Book> books_;
};

} // namespace chimera

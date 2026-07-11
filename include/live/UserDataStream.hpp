#pragma once
// ============================================================================
// UserDataStream — the authenticated execution-report path. (Phase-2 review
// fix item 8, 2026-07-11.)
//
// LIVE: Binance pushes executionReport events (fills / partials / cancels /
// fees) over a user-data websocket keyed by a listenKey that must be renewed
// (keepalive) every < 30 min or it expires. This manager owns the listenKey
// lifecycle and turns each event into an ExecReport that drives the
// ExchangeLedger — so holdings update from the exchange's truth, not intent.
//
// A REST fallback reconcile (query recent trades / open orders) covers gaps if
// the stream drops, so a missed event is caught on the next poll.
//
// SHADOW: there is no real stream to connect. The code path is wired and
// ACTIVATES LIVE; in shadow it is driven by simulated reports via feed_report()
// (called by the gateway right after a shadow fill). Clearly marked so the same
// ledger-driving logic is exercised now and unchanged at go-live.
// ============================================================================
#include <string>
#include <functional>
#include <cstdint>
#include "live/ExchangeLedger.hpp"

namespace chimera {

class UserDataStream {
public:
    using ReportHandler = std::function<void(const ExecReport&)>;

    void set_handler(ReportHandler h) { handler_ = std::move(h); }

    // --- listenKey lifecycle (LIVE-activated) -------------------------------
    // In live these call BinanceREST create/keepalive/close; here we hold the
    // state machine so renewal timing is testable and the wiring is explicit.
    void set_listen_key(const std::string& k, int64_t now_ms) {
        listen_key_ = k; last_keepalive_ms_ = now_ms; active_ = !k.empty();
    }
    const std::string& listen_key() const { return listen_key_; }
    bool active() const { return active_; }

    // Renew every < 30 min (Binance expiry is 60 min; renew at 30 for margin).
    bool needs_keepalive(int64_t now_ms) const {
        return active_ && (now_ms - last_keepalive_ms_) >= keepalive_interval_ms_;
    }
    void mark_keepalive(int64_t now_ms) { last_keepalive_ms_ = now_ms; }
    void close() { active_ = false; listen_key_.clear(); }

    // --- report ingestion (LIVE stream OR shadow simulation) ----------------
    // The single entry point every execution report flows through. LIVE: called
    // from the WS event parser. SHADOW: called by the gateway with a simulated
    // report. Either way it drives the ledger via the handler.
    void feed_report(const ExecReport& r) { if (handler_) handler_(r); ++reports_; }
    uint64_t reports() const { return reports_; }

    // Marker so logs/GUI can state honestly that live streaming is not connected.
    bool live_connected() const { return active_ && !shadow_driven_; }
    void set_shadow_driven(bool s) { shadow_driven_ = s; }

private:
    ReportHandler handler_;
    std::string   listen_key_;
    bool          active_ = false;
    bool          shadow_driven_ = true;
    int64_t       last_keepalive_ms_ = 0;
    int64_t       keepalive_interval_ms_ = 30 * 60 * 1000;
    uint64_t      reports_ = 0;
};

} // namespace chimera

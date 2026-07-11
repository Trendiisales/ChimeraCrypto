#pragma once
// ============================================================================
// OrderIdRegistry — deterministic client order IDs + in-flight tracking +
// unknown-order recovery. (Phase-2 review fix item 7, 2026-07-11.)
//
// BEFORE: SpotExecutor minted a client id from the microsecond clock, so a retry
// after an ambiguous send (timeout, dropped ACK) produced a DIFFERENT id and
// could double-buy. There was no "is this order already in flight?" check.
//
// AFTER: make_client_id(source, signal_id, symbol, is_buy) is a pure function of
// the order's intent, so a retry reproduces the SAME id — idempotent at the
// exchange (Binance dedups newClientOrderId within a symbol). The registry tracks
// each id's state + timestamp; after a timeout the caller MUST query-by-id first
// (recover) and only resubmit if the exchange confirms the order is absent.
// NEVER blind-resubmit.
//
// Header-only, no REST dependency — the query is done by the caller via
// BinanceREST::query_order and fed back through on_query_result().
// ============================================================================
#include <string>
#include <map>
#include <cstdint>

namespace chimera {

enum class RecoveryAction { WAIT, RECOVER_QUERY_FIRST, SAFE_TO_SUBMIT };

class OrderIdRegistry {
public:
    void set_timeout_ms(int64_t t) { timeout_ms_ = t; }

    // Deterministic, <=36 chars (Binance newClientOrderId cap). Encodes the intent
    // so an identical retry reproduces the identical id.
    static std::string make_client_id(const std::string& source, uint64_t signal_id,
                                      const std::string& symbol, bool is_buy) {
        // compact: 4-char source tag + action + symbol prefix + base36(signal_id)
        std::string src = source;
        for (auto& c : src) if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (src.size() > 4) src = src.substr(0, 4);
        std::string sym = symbol;
        for (auto& c : sym) if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (sym.size() > 8) sym = sym.substr(0, 8);
        std::string id = src + (is_buy ? "B" : "S") + sym + "-" + base36(signal_id);
        if (id.size() > 36) id = id.substr(0, 36);
        return id;
    }

    // Record that we are about to submit `id`. Returns SAFE_TO_SUBMIT the first
    // time; if the id is already in flight it returns RECOVER_QUERY_FIRST (never
    // blind-resubmit a live/unknown id).
    RecoveryAction on_submit(const std::string& id, int64_t now_ms) {
        auto it = inflight_.find(id);
        if (it == inflight_.end()) {
            inflight_[id] = Rec{now_ms, State::SENT};
            return RecoveryAction::SAFE_TO_SUBMIT;
        }
        return RecoveryAction::RECOVER_QUERY_FIRST;
    }

    // Mark the outcome of a submit that returned a definite ACK/fill/reject.
    void on_result(const std::string& id, bool accepted) {
        auto it = inflight_.find(id);
        if (it == inflight_.end()) return;
        it->second.state = accepted ? State::ACKED : State::DONE;
        if (!accepted) inflight_.erase(it);   // rejected: id is free to reuse
    }

    // A send that timed out / lost its ACK. The id is now UNKNOWN — the caller
    // must query the exchange before doing anything else.
    void on_timeout(const std::string& id, int64_t now_ms) {
        auto it = inflight_.find(id);
        if (it == inflight_.end()) { inflight_[id] = Rec{now_ms, State::UNKNOWN}; return; }
        it->second.state = State::UNKNOWN;
    }

    // Decide what to do about an id we hold. RECOVER_QUERY_FIRST once it is UNKNOWN
    // or an ACKED order has been outstanding beyond the timeout.
    RecoveryAction decide(const std::string& id, int64_t now_ms) const {
        auto it = inflight_.find(id);
        if (it == inflight_.end()) return RecoveryAction::SAFE_TO_SUBMIT;
        if (it->second.state == State::UNKNOWN) return RecoveryAction::RECOVER_QUERY_FIRST;
        if (it->second.state == State::DONE)    return RecoveryAction::WAIT; // filled/closed
        if (now_ms - it->second.ts_ms > timeout_ms_) return RecoveryAction::RECOVER_QUERY_FIRST;
        return RecoveryAction::WAIT;
    }

    // Feed the result of a query-by-id. found=true => the order EXISTS on the
    // exchange (do NOT resubmit — it is live/filled). found=false => it never
    // landed and is now safe to (re)submit.
    RecoveryAction on_query_result(const std::string& id, bool found) {
        auto it = inflight_.find(id);
        if (found) { if (it != inflight_.end()) it->second.state = State::ACKED; return RecoveryAction::WAIT; }
        if (it != inflight_.end()) inflight_.erase(it);   // absent -> free to resubmit
        return RecoveryAction::SAFE_TO_SUBMIT;
    }

    bool in_flight(const std::string& id) const { return inflight_.count(id) != 0; }
    size_t size() const { return inflight_.size(); }

private:
    enum class State { SENT, ACKED, UNKNOWN, DONE };
    struct Rec { int64_t ts_ms = 0; State state = State::SENT; };

    static std::string base36(uint64_t v) {
        static const char* D = "0123456789abcdefghijklmnopqrstuvwxyz";
        if (v == 0) return "0";
        std::string s; while (v) { s += D[v % 36]; v /= 36; }
        for (size_t i = 0, j = s.size() - 1; i < j; ++i, --j) std::swap(s[i], s[j]);
        return s;
    }

    std::map<std::string, Rec> inflight_;
    int64_t timeout_ms_ = 5000;
};

} // namespace chimera

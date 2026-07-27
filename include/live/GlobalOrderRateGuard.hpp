#pragma once
// =============================================================================
// GlobalOrderRateGuard.hpp — DESK-WIDE order-rate + new-position-fan-out caps.
//
// PORTED FROM OMEGA (include/GlobalOrderRateGuard.hpp, S-2026-07-27m) under the
// operator rule `feedback-safety-fix-both-systems-default`: an exec-path safety fix
// applies to BOTH systems in the same change. It did not, in S-27m — that pass
// grepped /Users/jo/Crypto (a research repo) and concluded "no crypto chokepoint
// exists". This repo IS the live crypto system, and it HAS one:
// `ExecutionGatewayT<SpotExecutor>::submit` (src/main.cpp:3511).
//
// THE TWO DEFECTS THIS FIXES, BOTH MEASURED IN THIS FILE'S SIBLING
// ----------------------------------------------------------------
//   include/live/ExecutionGateway.hpp:623  static constexpr int MAX_ORDERS_PER_SEC = 25;
//   include/live/ExecutionGateway.hpp:326  if (now_cb - rate_win_start_ms_ >= 1000)
//                                          { rate_win_start_ms_ = now_cb; rate_win_count_ = 0; }
//
// (1) A RESET-EVERY-SECOND BUCKET IS NOT A RATE LIMIT. It admits up to 2N orders
//     across a bucket boundary — send N at t=0.999s and N at t=1.001s and both pass,
//     because the counter was zeroed in between. The storm shape is precisely a burst
//     that straddles boundaries. A SLIDING window over timestamps cannot be reset by
//     the passage of time; only by the orders themselves ageing out.
//
// (2) NOTHING BOUNDED HOW MANY *POSITIONS* THE DESK OPENS AT ONCE. Every other
//     control here is keyed per symbol, and six symbols each sending exactly one
//     order is unremarkable to all of them. On the Omega side that is literally what
//     happened on 2026-07-24: AMD, CRWD, MU, PANW, STX and QQQ all opened inside ~6
//     seconds on a ~$12k account, and the account was force-liquidated that afternoon.
//     Only a desk-wide view can see a fan-out.
//
// DESIGN — identical to the Omega original, so the two systems fail the same way:
//   * Sliding windows over TIMESTAMPS, never a reset bucket.
//   * COUNT EVERYTHING, BLOCK ONLY WHAT ADDS RISK. An exit is counted (so a flatten
//     storm is visible and escalates) but NEVER blocked — an exit that cannot fire is
//     how a position becomes permanent. Entries fail CLOSED.
//   * No silent drops: over the cap => refuse, loudly, with the count in the reason.
//
// DELIBERATE DIFFERENCE FROM OMEGA: this guard REFUSES the individual order; it does
// not trip the gateway's sticky circuit-breaker. The sticky trip stays on the coarse
// 25/sec runaway limit. Refusing one order at 9/sec and hard-halting the whole desk
// until a human restarts it are very different consequences, and the tighter cap must
// not inherit the harsher one.
// =============================================================================
#include <string>
#include <deque>
#include <set>
#include <mutex>
#include <cstdio>
#include <cstdint>
#include <chrono>
#include <utility>

namespace chimera {

class GlobalOrderRateGuard {
public:
    // Caps: generous versus a real desk (a handful of orders a minute across the whole
    // book), brutally tight versus the observed storm shape.
    int  max_orders_per_sec_   = 8;    // ALL symbols, ALL kinds, 1s sliding
    int  max_orders_per_min_   = 40;   // ALL symbols, 60s sliding
    int  max_new_syms_per_min_ = 4;    // DISTINCT symbols newly opened, 60s sliding
    bool enabled_              = true;

    struct Verdict { bool allow = true; std::string why; };

    // `risk_reducing` = this order closes or reduces an existing position.
    // `new_symbol`    = this order opens exposure in a symbol not currently held.
    Verdict admit(const std::string& sym, bool risk_reducing, bool new_symbol) {
        if (!enabled_) return {};
        const int64_t now = now_ms_();
        std::lock_guard<std::mutex> lk(m_);
        prune_(now);

        const int n_sec = (int)sec_.size();
        const int n_min = (int)min_.size();

        // ── risk-reducing orders: COUNTED, NEVER BLOCKED ────────────────────────
        if (risk_reducing) {
            sec_.push_back(now); min_.push_back(now);
            if (n_sec + 1 > max_orders_per_sec_ || n_min + 1 > max_orders_per_min_) {
                std::fprintf(stderr,
                    "[SYSTEM-ALERT] [ORDER-RATE] desk-wide rate EXCEEDED on a RISK-REDUCING "
                    "order (%s): %d in 1s, %d in 60s (caps %d/%d). NOT blocked -- an exit must "
                    "always be able to fire -- but this is the shape of a flatten storm. "
                    "Investigate the loop that is issuing them.\n",
                    sym.c_str(), n_sec + 1, n_min + 1,
                    max_orders_per_sec_, max_orders_per_min_);
                std::fflush(stderr);
            }
            return {};
        }

        // ── entries / adds: FAIL CLOSED ─────────────────────────────────────────
        if (n_sec + 1 > max_orders_per_sec_) {
            return {false, "desk-wide " + std::to_string(n_sec + 1) + " orders in 1s (cap " +
                           std::to_string(max_orders_per_sec_) + ") -- ALL symbols combined"};
        }
        if (n_min + 1 > max_orders_per_min_) {
            return {false, "desk-wide " + std::to_string(n_min + 1) + " orders in 60s (cap " +
                           std::to_string(max_orders_per_min_) + ") -- ALL symbols combined"};
        }
        if (new_symbol) {
            // Count DISTINCT symbols, not orders: three adds to one name is one
            // decision; one order each in three names is three decisions.
            std::set<std::string> distinct;
            for (const auto& e : new_syms_) distinct.insert(e.second);
            if (!distinct.count(sym) && (int)distinct.size() + 1 > max_new_syms_per_min_) {
                std::string names;
                for (const auto& s : distinct) { if (!names.empty()) names += ","; names += s; }
                return {false, "desk-wide position FAN-OUT: " + std::to_string(distinct.size()) +
                               " new symbol(s) opened in the last 60s (" + names + ") + " + sym +
                               " would be " + std::to_string(distinct.size() + 1) + " (cap " +
                               std::to_string(max_new_syms_per_min_) + ")"};
            }
            new_syms_.push_back({now, sym});
        }
        sec_.push_back(now); min_.push_back(now);
        return {};
    }

    // Count an order that did NOT go through admit() -- e.g. the separate
    // emergency_flatten path. Counting-only, never blocking: the whole point is that
    // a flatten storm becomes VISIBLE to the desk-wide view instead of invisible to
    // it, which is the bypass `close_broker_position` had on the Omega side until
    // S-27m (`feedback-separate-binary-bypasses-guards`).
    void note_out_of_band(const std::string& sym) {
        if (!enabled_) return;
        const int64_t now = now_ms_();
        std::lock_guard<std::mutex> lk(m_);
        prune_(now);
        sec_.push_back(now); min_.push_back(now);
        if ((int)sec_.size() > max_orders_per_sec_ || (int)min_.size() > max_orders_per_min_) {
            std::fprintf(stderr,
                "[SYSTEM-ALERT] [ORDER-RATE] out-of-band order path (%s) pushed the desk-wide "
                "rate over cap: %zu in 1s, %zu in 60s (caps %d/%d). Not blocked (this path is "
                "risk-reducing by construction) -- but it is now COUNTED, so a flatten storm "
                "cannot hide from the desk-wide view.\n",
                sym.c_str(), sec_.size(), min_.size(),
                max_orders_per_sec_, max_orders_per_min_);
            std::fflush(stderr);
        }
    }

    // Orders in the last `ms` milliseconds, sliding. Used by the gateway to evaluate
    // its coarse runaway limit on a sliding window instead of a reset bucket.
    int count_in_last_ms(int64_t ms) {
        std::lock_guard<std::mutex> lk(m_);
        const int64_t now = now_ms_();
        prune_(now);
        int n = 0;
        for (auto it = min_.rbegin(); it != min_.rend(); ++it) {
            if (now - *it >= ms) break;
            ++n;
        }
        return n;
    }

    std::string summary() {
        std::lock_guard<std::mutex> lk(m_);
        prune_(now_ms_());
        char b[192];
        std::snprintf(b, sizeof b, "1s=%zu/%d 60s=%zu/%d newsyms60s=%zu/%d",
                      sec_.size(), max_orders_per_sec_, min_.size(), max_orders_per_min_,
                      new_syms_.size(), max_new_syms_per_min_);
        return b;
    }

private:
    static int64_t now_ms_() {
        return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    void prune_(int64_t now) {                       // caller holds m_
        while (!sec_.empty()      && now - sec_.front()           >= 1000)  sec_.pop_front();
        while (!min_.empty()      && now - min_.front()           >= 60000) min_.pop_front();
        while (!new_syms_.empty() && now - new_syms_.front().first >= 60000) new_syms_.pop_front();
    }
    std::mutex m_;
    std::deque<int64_t> sec_, min_;
    std::deque<std::pair<int64_t,std::string>> new_syms_;
};

} // namespace chimera

#pragma once
// =============================================================================
// SlowStormGuard.hpp — THE STORM THAT TAKES ALL DAY.
//
// PORTED FROM OMEGA (include/SlowStormGuard.hpp, S-2026-07-27m gate 16) under
// `feedback-safety-fix-both-systems-default`.
//
// WHY THE EXISTING CRYPTO BREAKER CANNOT SEE IT
// ---------------------------------------------
// `ExecutionGateway.hpp` has, measured from source:
//
//   :623  static constexpr int MAX_UNFILLED_PER_SYM = 8;
//   :385  unfilled_by_sym_[in.symbol] = 0;        // "a fill zeroes the counter"
//
// Two independent blind spots follow from those two lines:
//
//  (a) A STORM THAT FILLS RESETS ITS OWN BREAKER. Every filled order zeroes the
//      counter, so a loop whose orders keep filling can run forever. On the Omega
//      side that is exactly what happened: twelve 1-share QQQ sells (oids 240-251),
//      every one FILLED, leaving a naked -11 short. STATED PLAINLY AND NOT PAPERED
//      OVER: this guard does NOT fix that case, because the same reset lives here by
//      design ("a filling symbol is trading, not looping"). The filling case is
//      bounded by GlobalOrderRateGuard's sliding desk-wide window instead. Do not
//      credit this guard for it.
//
//  (b) A SLOW DRIP IS INVISIBLE TO A BURST BREAKER. The Omega tape for 2026-07-27
//      shows ~18 `Sell 2 AMD Market/Day` between 08:58 and 09:30, all CANCELLED,
//      zero filled; the same for DELL; five `Buy 11 QQQ` across 90 minutes; and
//      protective stops placed-and-cancelled fourteen times over the day. Eighteen
//      unfilled sends spread over 30 MINUTES never put 8 inside any 30-SECOND
//      window. That loop churned every per-day budget to exhaustion and is the
//      direct reason the operator's own close was refused. THAT is what this guard
//      is for.
//
// TWO CONTROLS, NEITHER REPLACING THE BURST BREAKER
// -------------------------------------------------
//  1. CUMULATIVE UNFILLED, PER SYMBOL, PER UTC DAY, NO DECAY. A fill resets it. A
//     cancel or a reject does NOT — those are the loop's own output, and resetting
//     on them is how a decaying counter erases its own evidence.
//  2. EXPONENTIAL BACKOFF PER (symbol, intent). Retrying an order the exchange keeps
//     refusing is a loop, not resilience. Consecutive failures push the next
//     permitted attempt out geometrically; after max_consecutive_ the guard STOPS
//     and says so once.
//
// WHAT IS DELIBERATELY NOT BLOCKED
// --------------------------------
// An OPERATOR-originated risk-reducing order is never delayed or refused. Automated
// risk-reducing retries ARE backed off — those are the loop — but never permanently
// stopped, so a transient exchange condition still self-heals.
//
// CRYPTO-SPECIFIC NOTE: the book is long-only spot, so "risk-reducing" == a SELL of
// held base. There is no short leg whose exit is a buy.
// =============================================================================
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <ctime>

namespace chimera {

class SlowStormGuard {
public:
    enum Origin { AUTOMATED = 0, OPERATOR = 1 };

    int     max_unfilled_per_sym_day_ = 12;
    int     max_consecutive_          = 6;
    int64_t backoff_base_ms_          = 60000;     // 1 min, doubling
    int64_t backoff_cap_ms_           = 3600000;   // 1 hour ceiling
    bool    enabled_                  = true;

    struct Verdict { bool allow = true; std::string why; };

    // intent: a short stable string identifying WHAT is being retried, e.g.
    // "EXIT", "OPEN", "STOP". (symbol,intent) is the retry identity.
    Verdict admit(const std::string& sym, const std::string& intent,
                  bool risk_reducing, Origin origin) {
        if (!enabled_) return {};
        if (risk_reducing && origin == OPERATOR) return {};
        const int64_t now = now_ms_();
        std::lock_guard<std::mutex> lk(m_);
        roll_day_();
        const std::string key = sym + "|" + intent;
        Retry& r = retry_[key];

        if (r.consecutive >= max_consecutive_) {
            if (!r.stopped_announced) {
                r.stopped_announced = true;
                std::fprintf(stderr,
                    "[SYSTEM-ALERT] [SLOW-STORM] %s %s STOPPED after %d consecutive failed "
                    "attempts. Retrying an order the exchange keeps refusing is a loop, not "
                    "resilience -- on the Omega side the same shape re-sent a refused order "
                    "every few minutes for twelve hours and burned every per-day budget doing "
                    "it. A FILL on this symbol clears it; otherwise this needs MANUAL action.\n",
                    sym.c_str(), intent.c_str(), r.consecutive);
                std::fflush(stderr);
            }
            // A risk-reducing order is SLOWED, never permanently stopped.
            if (!risk_reducing)
                return {false, "retry loop stopped after " + std::to_string(r.consecutive) +
                               " consecutive failures"};
        }
        if (r.next_allowed_ms > now) {
            const long long wait_s = (r.next_allowed_ms - now) / 1000;
            if (!r.backoff_announced) {
                r.backoff_announced = true;
                std::fprintf(stderr,
                    "[SLOW-STORM] %s %s backing off %llds after %d consecutive failure(s) -- "
                    "geometric, so a persistent refusal cannot be re-sent every minute all day\n",
                    sym.c_str(), intent.c_str(), wait_s, r.consecutive);
                std::fflush(stderr);
            }
            return {false, "backing off " + std::to_string(wait_s) + "s after " +
                           std::to_string(r.consecutive) + " consecutive failure(s)"};
        }
        int& u = unfilled_today_[sym];
        if (u >= max_unfilled_per_sym_day_) {
            if (!day_announced_[sym]) {
                day_announced_[sym] = true;
                std::fprintf(stderr,
                    "[SYSTEM-ALERT] [SLOW-STORM] %s has %d unfilled order(s) today (cap %d). "
                    "This is the CUMULATIVE counter -- a burst breaker with a 30s window is "
                    "structurally blind to a loop that retries every few minutes for hours. "
                    "A FILL resets it; a cancel or reject does not.\n",
                    sym.c_str(), u, max_unfilled_per_sym_day_);
                std::fflush(stderr);
            }
            if (!risk_reducing)
                return {false, std::to_string(u) + " unfilled orders today (cap " +
                               std::to_string(max_unfilled_per_sym_day_) + ")"};
        }
        r.backoff_announced = false;
        return {};
    }

    // A send went out. Counted as unfilled until a fill says otherwise.
    void on_send(const std::string& sym) {
        if (!enabled_) return;
        std::lock_guard<std::mutex> lk(m_);
        roll_day_();
        ++unfilled_today_[sym];
    }

    // Terminal failure (rejected / cancelled / expired, nothing filled).
    void on_fail(const std::string& sym, const std::string& intent) {
        if (!enabled_) return;
        const int64_t now = now_ms_();
        std::lock_guard<std::mutex> lk(m_);
        Retry& r = retry_[sym + "|" + intent];
        ++r.consecutive;
        int64_t d = backoff_base_ms_;
        for (int i = 1; i < r.consecutive && d < backoff_cap_ms_; ++i) d *= 2;
        if (d > backoff_cap_ms_) d = backoff_cap_ms_;
        r.next_allowed_ms   = now + d;
        r.backoff_announced = false;
    }

    // A real fill. The symbol is trading, so its retry latches and day counter are
    // genuinely stale. See blind spot (a) in the header: this is DELIBERATE, and it
    // is why this guard is not the control that bounds a FILLING storm.
    void on_fill(const std::string& sym) {
        if (!enabled_) return;
        std::lock_guard<std::mutex> lk(m_);
        unfilled_today_[sym] = 0;
        day_announced_[sym]  = false;
        for (auto it = retry_.begin(); it != retry_.end(); ++it)
            if (it->first.rfind(sym + "|", 0) == 0) it->second = Retry{};
    }

    std::string summary() {
        std::lock_guard<std::mutex> lk(m_);
        char b[160];
        std::snprintf(b, sizeof b, "day=%s syms=%zu retry-keys=%zu caps unfilled/day=%d consecutive=%d",
                      day_.c_str(), unfilled_today_.size(), retry_.size(),
                      max_unfilled_per_sym_day_, max_consecutive_);
        return b;
    }

private:
    struct Retry {
        int     consecutive       = 0;
        int64_t next_allowed_ms   = 0;
        bool    backoff_announced = false;
        bool    stopped_announced = false;
    };
    static int64_t now_ms_() {
        return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    static std::string utc_day_() {
        std::time_t t = std::time(nullptr); std::tm g{};
#ifdef _WIN32
        gmtime_s(&g, &t);
#else
        gmtime_r(&t, &g);
#endif
        char b[16];
        std::snprintf(b, sizeof b, "%04d-%02d-%02d", g.tm_year + 1900, g.tm_mon + 1, g.tm_mday);
        return b;
    }
    void roll_day_() {                              // caller holds m_
        const std::string d = utc_day_();
        if (d == day_) return;
        day_ = d;
        unfilled_today_.clear();
        day_announced_.clear();
        // Retry backoff state is NOT cleared on a day roll. An exchange that refused an
        // order all day will refuse it tomorrow; only a FILL is evidence it cleared.
    }
    std::mutex m_;
    std::string day_;
    std::map<std::string,int>   unfilled_today_;
    std::map<std::string,bool>  day_announced_;
    std::map<std::string,Retry> retry_;
};

} // namespace chimera

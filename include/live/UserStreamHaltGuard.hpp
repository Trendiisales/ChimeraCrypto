#pragma once
// ============================================================================
// UserStreamHaltGuard — AUTO-arm the entry halt when the LIVE user-data-stream
// heartbeat lapses, and hold it until a clean reconcile clears it.
// (Phase-8G review — go-live blocker, 2026-07-11.)
//
// THE GAP THIS CLOSES. Phase-2 built the halt-until-reconcile pathway
// (`StartupReconciler` + the `ExecutionGateway` kill-switch) and the CI
// `stale_user_stream_test` proved it — but that test DRIVES the halt explicitly.
// The LIVE loop did NOT yet AUTO-arm the halt the instant the exchange
// user-data-stream heartbeat lapsed, so a silent stream gap at go-live would
// leave new entries flowing without fill/position truth. This latch is the
// missing auto-trigger.
//
// WIRING (reuse, don't duplicate). The one gateway's `kill_switch_active`
// already blocks entries while true and always passes EXITS. This guard is OR'd
// into that predicate:
//   * poll(stream, now) — call each time entries are gated (the gateway calls it
//     from kill_switch_active). If the stream's heartbeat has lapsed beyond the
//     threshold it LATCHES `halted_` on. Latched = stays halted even if a stray
//     late heartbeat arrives; only a reconcile clears it.
//   * on_reconcile(passed) — a CLEAN StartupReconciler pass (ledger re-agrees
//     with the exchange) auto-CLEARS the latch => entries resume. A failed /
//     mismatched reconcile leaves it armed.
//
// SHADOW no-op / LIVE-arm. poll() arms ONLY via UserDataStream::heartbeat_lapsed,
// which is false for a shadow-driven stream (no live stream to drop). So in
// shadow the latch never arms and the guard is completely inert; it ARMS on the
// live path once the real WS user-stream is connected at go-live. Header-only,
// no dependency beyond UserDataStream — trivially unit-testable.
// ============================================================================
#include <cstdint>
#include <cstdio>
#include "live/UserDataStream.hpp"

namespace chimera {

class UserStreamHaltGuard {
public:
    // Threshold for "the live stream heartbeat has lapsed" (default 45s; Binance
    // user-data streams heartbeat far more frequently than this).
    void    set_threshold_ms(int64_t ms) { if (ms > 0) threshold_ms_ = ms; }
    int64_t threshold_ms() const { return threshold_ms_; }

    // Poll the guard against the live stream at time now_ms. Latches the halt on
    // if the stream heartbeat has lapsed past the threshold. Returns the current
    // halted state. NO-OP in shadow (heartbeat_lapsed() is false for a
    // shadow-driven stream), so this never arms unless a real live stream drops.
    bool poll(const UserDataStream& stream, int64_t now_ms) {
        if (!halted_ && stream.heartbeat_lapsed(now_ms, threshold_ms_)) {
            halted_ = true; ++arm_count_;
            std::fprintf(stderr,
                "[STREAM-HALT] user-data stream heartbeat lapsed > %lldms — AUTO-HALT armed "
                "(new entries blocked, exits pass; clears on clean reconcile)\n",
                (long long)threshold_ms_);
        }
        return halted_;
    }

    // A clean reconcile (ledger re-agrees with the exchange) clears the latch;
    // a failed/mismatched one keeps it armed.
    void on_reconcile(bool reconcile_passed) {
        if (halted_ && reconcile_passed) {
            halted_ = false; ++clear_count_;
            std::fprintf(stderr,
                "[STREAM-HALT] clean reconcile — AUTO-HALT cleared, entries resume\n");
        }
    }

    bool     halted() const { return halted_; }
    uint64_t arm_count() const { return arm_count_; }
    uint64_t clear_count() const { return clear_count_; }

    // Operator/manual override hooks (used by tests + a manual desk halt).
    void arm()   { if (!halted_) { halted_ = true; ++arm_count_; } }
    void clear() { if (halted_)  { halted_ = false; ++clear_count_; } }

private:
    bool     halted_ = false;
    int64_t  threshold_ms_ = 45'000;   // 45s default
    uint64_t arm_count_ = 0;
    uint64_t clear_count_ = 0;
};

} // namespace chimera

#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// MimicCompanionEngine — STANDALONE ADDITIVE clip book (S-2026-07-03, Slice 4b).
//
// Observes ONE MIMIC parent leg (EdgeEngine, StrategyKind::MIMIC, ride-to-flip)
// per completed H1 bar and runs its OWN independent clip contract alongside it.
//
// HARD OPERATOR RULE — the companion is a SEPARATE, INDEPENDENT engine. It does
// NOT modify / close / move / shrink the parent Mimic position. The parent rides
// to symmetric down-jump flip (WIDE) regardless; the companion runs its own book.
// Judge STANDALONE (net-positive after cost, WF both halves, both regimes),
// NEVER vs-WIDE (Memory-Omega/wiki/entities/CompanionDominanceError.md,
// auto-memory feedback-companion-independent-engine).
//
// Faithful native port of stall_accountant.py clip decisions (the %-gauge path):
//   arm at peak  -> clip on STALL (N bars no new fav high) OR REVERSAL (giveback
//   fraction of peak) -> RECLIP (re-arm after each clip on a new fav high past the
//   prior peak). Long-only (MIMIC is always long). No cold-loss cut — the clip
//   itself (arm/stall/reversal/reclip) IS the protection (COLD_LOSS OFF in cron).
//
// Cost 0.20% RT = 20bp (Binance spot taker 0.10%/side, no BNB discount), deducted
// as net_bp = gross_bp - round_trip_bp, same convention as EdgeEngine exits.
//
// PAPER / SHADOW: emits its own ClipRecord ledger only. Never places an order,
// never calls back into the parent. Additive.
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <cstdint>
#include <functional>
#include <cstdio>
#include <utility>

namespace chimera {

class MimicCompanionEngine {
public:
    struct Config {
        std::string parent_tag;        // e.g. "BTC-MIMIC-H1" (the leg we observe)
        std::string tag;               // e.g. "BTC-MIMIC-CLIP" (our own ledger tag)
        std::string symbol;            // e.g. "btcusdt"
        double  arm_pct       = 5.0;   // profit-gate: arm triggers once mfe% >= this
        int     stall_bars    = 6;     // clip after N bars with no new fav high (0 = stall OFF)
        double  rev_gb        = 0.0;   // clip when fav <= mfe*(1-rev_gb)      (0 = reversal OFF)
        double  reclip_pct    = 0.0;   // re-arm when fav > prior_peak*(1+reclip_pct) (0 = single-clip)
        int64_t tf_secs       = 3600;  // H1
        double  round_trip_bp = 20.0;  // 0.20% RT Binance spot taker
    };

    // Emitted on every clip / engine-exit. main.cpp persists to the companion ledger.
    struct ClipRecord {
        std::string tag, symbol, reason;   // STALL_CLIP / REVERSAL_CLIP / ENGINE_EXIT
        int64_t entry_ts_ms = 0, exit_ts_ms = 0;
        double  entry_px = 0.0, exit_px = 0.0;
        double  gross_bp = 0.0, net_bp = 0.0, mfe_pct = 0.0;
        int     bars_held = 0, clip_num = 0;
        bool    shadow = true;
    };
    using ClipCallback = std::function<void(const ClipRecord&)>;

    explicit MimicCompanionEngine(Config c) : cfg_(std::move(c)) {}

    void set_on_clip(ClipCallback cb) { on_clip_ = std::move(cb); }
    const Config& config() const { return cfg_; }
    bool  is_open() const { return open_; }
    int   clips()   const { return clip_num_; }
    // Restore cumulative counters from the durable clip log on boot so the desk
    // panel clips/bank_bp survive a process restart (otherwise in-RAM -> 0 each boot).
    // Per-trade session state (mfe/open/stall) intentionally stays ephemeral.
    void rehydrate(int clips_total, double bank_bp_total) { clip_num_ = clips_total; banked_bp_ = bank_bp_total; }

    // Rehydrate an OPEN companion session from a live parent position on process
    // restart (S-2026-07-05). Without this, observe() only runs on completed H1
    // bars, so after a restart the companion is dark until the next H1 close and
    // then re-anchors its peak to the CURRENT bar's fav — losing the parent's true
    // peak-to-date (a deep-in-profit leg wrongly shows armed=false meanwhile).
    // seed_open() sets the session open immediately and restores the peak from the
    // parent's mfe_px. Called from main.cpp AFTER resume_position() (so entry_px /
    // mfe_px / entry_ts are populated). It fully aligns entry_ref_ with the parent
    // entry, so the very next observe() does NOT reset the session.
    //   entry_px    — parent entry for the live trade
    //   entry_ts_ms — parent entry ts (open_bar_ -> correct bars_held on later clips)
    //   peak_px     — parent mfe_px (highest fav price seen); <=entry => peak 0%
    //   now_ms      — current time; the STALL counter re-anchors here (the peak's ts
    //                 is not persisted, so we start stall fresh from the restart to
    //                 avoid a spurious instant stall-clip on boot). Documented reset.
    void seed_open(double entry_px, int64_t entry_ts_ms, double peak_px, int64_t now_ms) {
        if (open_ || clipped_ || entry_px <= 0.0) return;
        entry_ref_  = entry_px;
        open_       = true;
        open_ts_    = entry_ts_ms;
        open_bar_   = entry_ts_ms / (cfg_.tf_secs * 1000);
        mfe_pct_    = (peak_px > entry_px) ? (peak_px / entry_px - 1.0) * 100.0 : 0.0;
        ext_bar_    = now_ms / (cfg_.tf_secs * 1000);   // stall fresh from restart (peak ts not persisted)
        stall_now_  = 0;
        prior_peak_ = 0.0;
    }
    bool  shadow_mode = true;

    // Live per-leg snapshot for the Omega desk CRYPTO COMPANIONS panel. Read-only
    // view of settled state (no side effects). sym is derived in main.cpp from tag.
    struct LiveSnap {
        bool   open            = false;   // companion session currently open
        bool   armed           = false;   // profit-gate cleared (mfe >= arm_pct)
        double peak_mfe_pct    = 0.0;     // peak favourable % since (re)open
        int    bars_since_high = 0;       // stall = bars since last new fav high
        int    clips           = 0;       // clips banked (durable: rehydrated from clip log on boot)
        double bank_bp         = 0.0;     // cumulative net_bp banked (after cost)
    };
    LiveSnap snapshot() const {
        LiveSnap s;
        s.open            = open_;
        s.armed           = open_ && (mfe_pct_ >= cfg_.arm_pct);
        s.peak_mfe_pct    = mfe_pct_;
        s.bars_since_high = stall_now_;
        s.clips           = clip_num_;
        s.bank_bp         = banked_bp_;
        return s;
    }

    // Drive ONCE per completed parent H1 bar. Reads the parent's settled position
    // state only (never writes to it). long-only: MIMIC is always a long.
    //   parent_in_pos   — engine.in_position() after the bar settled
    //   parent_entry_px — engine.entry_px()   (the parent's entry for THIS trade)
    //   cur_px          — the completed bar close
    //   ts_ms           — bar open ts (ms)
    void observe(bool parent_in_pos, double parent_entry_px, double cur_px, int64_t ts_ms) {
        const int64_t bar = ts_ms / (cfg_.tf_secs * 1000);

        // Parent flat (or no valid mark) -> bank any open companion as ENGINE_EXIT,
        // then reset the session so a future NEW parent trade tracks cleanly.
        if (!parent_in_pos || parent_entry_px <= 0.0 || cur_px <= 0.0) {
            if (open_) close_(cur_px > 0.0 ? cur_px : entry_ref_, ts_ms, bar, "ENGINE_EXIT");
            reset_session_();
            return;
        }

        // A NEW parent trade (entry price changed) -> reset companion session.
        if (entry_ref_ != parent_entry_px) { reset_session_(); entry_ref_ = parent_entry_px; }

        const double fav = (cur_px - parent_entry_px) / parent_entry_px * 100.0;

        if (clipped_) {
            // RECLIP: re-open only when the parent (still live) makes a NEW favourable
            // high past the prior clip peak by reclip_pct. Else stay clipped (dark).
            if (cfg_.reclip_pct > 0.0 && prior_peak_ > 0.0 &&
                fav > prior_peak_ * (1.0 + cfg_.reclip_pct)) {
                clipped_ = false;                 // fall through -> re-arm a fresh companion
            } else {
                return;
            }
        }

        if (!open_) {                             // open on first observation of this leg
            open_ = true; open_ts_ = ts_ms; open_bar_ = bar;
            mfe_pct_ = fav; ext_bar_ = bar;
        }
        if (fav > mfe_pct_ + 1e-9) { mfe_pct_ = fav; ext_bar_ = bar; }   // new fav extreme

        const int  stall = static_cast<int>(bar - ext_bar_);
        stall_now_ = stall;                                              // live snapshot view
        const bool armed = mfe_pct_ >= cfg_.arm_pct;                     // profit-gate cleared

        if (armed && cfg_.stall_bars > 0 && stall >= cfg_.stall_bars) {  // stagnation
            prior_peak_ = mfe_pct_; close_(cur_px, ts_ms, bar, "STALL_CLIP"); clipped_ = true; return;
        }
        if (armed && cfg_.rev_gb > 0.0 && fav <= mfe_pct_ * (1.0 - cfg_.rev_gb)) {  // reversal
            prior_peak_ = mfe_pct_; close_(cur_px, ts_ms, bar, "REVERSAL_CLIP"); clipped_ = true; return;
        }
    }

private:
    void reset_session_() {
        open_ = false; clipped_ = false; prior_peak_ = 0.0;
        mfe_pct_ = 0.0; ext_bar_ = 0; open_bar_ = 0; open_ts_ = 0; entry_ref_ = 0.0;
    }

    void close_(double exit_px, int64_t ts_ms, int64_t bar, const char* reason) {
        ClipRecord r;
        r.tag = cfg_.tag; r.symbol = cfg_.symbol; r.reason = reason;
        r.entry_ts_ms = open_ts_; r.exit_ts_ms = ts_ms;
        r.entry_px = entry_ref_;  r.exit_px = exit_px;
        r.gross_bp = (entry_ref_ > 0.0) ? (exit_px / entry_ref_ - 1.0) * 1e4 : 0.0;
        r.net_bp   = r.gross_bp - cfg_.round_trip_bp;
        r.mfe_pct  = mfe_pct_;
        r.bars_held = static_cast<int>(bar - open_bar_);
        r.clip_num  = ++clip_num_;
        r.shadow    = shadow_mode;
        banked_bp_ += r.net_bp;                   // cumulative banked (after cost) for live snapshot
        if (on_clip_) on_clip_(r);
        std::printf("[CLIP][%s] %s net=%+.1fbp gross=%+.1fbp mfe=%.2f%% bars=%d px %.6f->%.6f shadow=%d\n",
            cfg_.tag.c_str(), reason, r.net_bp, r.gross_bp, r.mfe_pct, r.bars_held,
            r.entry_px, r.exit_px, shadow_mode ? 1 : 0);
        std::fflush(stdout);
        open_ = false;                            // closed; reclip may re-open next bar
    }

    Config       cfg_;
    ClipCallback on_clip_;
    bool    open_       = false;
    bool    clipped_    = false;
    double  entry_ref_  = 0.0;   // the parent entry this companion session tracks
    double  mfe_pct_    = 0.0;   // peak favourable % since (re)open
    double  prior_peak_ = 0.0;   // peak at last clip (for reclip gate)
    int64_t ext_bar_    = 0;     // bar of last new fav extreme
    int64_t open_bar_   = 0;
    int64_t open_ts_    = 0;
    int     clip_num_   = 0;
    int     stall_now_  = 0;     // live stall (bars since last new fav high) for snapshot
    double  banked_bp_  = 0.0;   // cumulative net_bp banked across clips (live snapshot)
};

} // namespace chimera

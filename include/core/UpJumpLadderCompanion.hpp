#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// UpJumpLadderCompanion — TIERED-2 + SELF-FUNDING LADDER clip book (S-2026-07-05b).
//
// Successor to UpJumpCompanionEngine (single-leg). Same STANDALONE ADDITIVE,
// observe-only, shadow contract — but every parent UPJUMP trade now runs a BOOK
// of independent clip legs:
//
//   • 2 BASE tiers from entry: a TIGHT tier (banks cost fast) + a WIDE tier
//     (rides far). ">= 2 engines per trade" is the operator floor.
//   • SELF-FUNDING LADDER: each time a leg banks a COST-COVERED clip (net_bp>0),
//     ONE more WIDE leg is opened at the clip price to ride the continuation,
//     up to `cap` concurrent legs total (2 base + up to cap-2 ladder). The clip
//     that funds it already paid its own cost (opt C — no free capital added).
//   • Each leg independently exits on STALL (N bars no new fav high) and/or
//     REVERSAL (giveback fraction of peak) — per-tier FREE lever (>=1 on) — and
//     RE-CLIPS (re-ENTERs at the current price) if the trend resumes.
//   • Optional HARD COST-COVER GATE (per-coin, e.g. AAVE): a leg may not bank a
//     clip whose gross does not clear RT cost; it keeps holding instead. The
//     parent-exit flush is ALWAYS marked-to-market (never abandoned) so no
//     underwater leg is hidden.
//
// HARD OPERATOR RULE — SEPARATE INDEPENDENT ENGINE. Never modifies / closes /
// moves / shrinks the parent. Judge STANDALONE (net>0, PF>1, WF both halves,
// bear>=0), NEVER vs-WIDE (feedback-companion-independent-engine).
//
// FAITHFUL byte-exact port of crypto_upjump_tiered_ladder_sweep.py (Leg + run_trade):
//   - fav / mfe / arm / reclip gauged from the leg's FIXED entry epx.
//   - clip gross_bp measured from the MOVING `le` (= epx, then reset to the clip
//     price on each reclip → "reclip = re-enter"). entry_px in the ClipRecord = le.
//   - ladder legs anchor epx=le=clip_price, WIDE params; newborn legs do NOT step
//     the bar they are born (added after the per-leg loop, exactly like python).
//   - flush open (not clipped) legs at the last observed price on parent exit.
//
// Cost 0.20% RT = 20bp (Binance spot taker). PAPER/SHADOW: emits its own
// ClipRecord ledger only, never places an order, never calls back into the parent.
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <cstdint>
#include <functional>
#include <cstdio>
#include <utility>
#include <vector>

namespace chimera {

class UpJumpLadderCompanion {
public:
    struct Tier { double arm = 5.0; int stall = 0; double gb = 0.0; };  // 0 = that lever OFF

    struct Config {
        std::string parent_tag;        // e.g. "BTC-UPJUMP-H1" (the leg we observe)
        std::string tag;               // e.g. "BTC-UPJUMP-CLIP" (our own ledger tag)
        std::string symbol;            // e.g. "btcusdt"
        Tier    tight;                 // base tier 1 (tight)
        Tier    wide;                  // base tier 2 (wide) — ALSO the ladder-leg params
        double  reclip_pct    = 0.05;  // re-enter when fav > prior_peak*(1+reclip_pct)
        int     cap           = 5;     // max concurrent legs (2 base + up to cap-2 ladder)
        double  cost_gate_bp  = 0.0;   // >0 = hard cost-cover clip gate (suppress sub-cost clips)
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

    explicit UpJumpLadderCompanion(Config c) : cfg_(std::move(c)) {}

    void set_on_clip(ClipCallback cb) { on_clip_ = std::move(cb); }
    const Config& config() const { return cfg_; }
    bool  is_open() const { for (auto& l : legs_) if (l.open) return true; return false; }
    int   clips()   const { return clip_num_; }
    void  rehydrate(int clips_total, double bank_bp_total) { clip_num_ = clips_total; banked_bp_ = bank_bp_total; }
    bool  shadow_mode = true;

    // ── one independent clip leg (faithful python Leg) ─────────────────────
    struct Leg {
        std::string label;      // "T1" / "T2" / "L1".. (tier / ladder id for the GUI)
        double  epx = 0.0;      // FIXED entry — fav/mfe/arm/reclip gauge
        double  le  = 0.0;      // MOVING leg entry — clip gross gauge (resets on reclip)
        double  arm = 5.0; int stall = 0; double gb = 0.0; double rc = 0.05; double cg = 0.0;
        bool    open = false, clipped = false;
        double  pk = 0.0, mfe = 0.0;
        int64_t ext_bar = 0, open_bar = 0, open_ts = 0;
    };

    // Drive ONCE per completed parent bar (byte-exact vs python) OR per tick
    // (intra-bar; bar index = ts/H1 only advances hourly so STALL stays H1-quantised
    // and REVERSAL/RECLIP price gates fire the instant they trip). Reads the parent's
    // settled position only, never writes to it. Long-only (UPJUMP is always long).
    void observe(bool parent_in_pos, double parent_entry_px, double cur_px, int64_t ts_ms) {
        const int64_t bar = ts_ms / (cfg_.tf_secs * 1000);

        // Parent flat / no valid mark -> flush every open leg MTM, then reset.
        if (!parent_in_pos || parent_entry_px <= 0.0 || cur_px <= 0.0) {
            const double px = (cur_px > 0.0) ? cur_px : entry_ref_;
            for (auto& lg : legs_) flush_leg_(lg, px, ts_ms, bar);
            reset_session_();
            return;
        }

        // New parent trade -> flush any stragglers, reset, seed the 2 base legs.
        if (entry_ref_ != parent_entry_px) {
            for (auto& lg : legs_) flush_leg_(lg, cur_px, ts_ms, bar);
            reset_session_();
            entry_ref_ = parent_entry_px;
            init_base_legs_(parent_entry_px, ts_ms, bar);
        }

        // Step every leg; ladder-spawn on cost-covered clips (newborns added AFTER
        // the loop so they do not step the bar they are born — matches python).
        std::vector<Leg> spawn;
        for (auto& lg : legs_) {
            double gross; const char* reason;
            if (step_leg_(lg, bar, cur_px, gross, reason)) {
                const double net = gross - cfg_.round_trip_bp;
                emit_clip_(lg, cur_px, ts_ms, bar, gross, net, reason);
                if (net > 0.0 && (int)(legs_.size() + spawn.size()) < cfg_.cap)
                    spawn.push_back(make_leg_(next_ladder_label_(legs_.size() + spawn.size()),
                                              cur_px, cfg_.wide, ts_ms, bar, /*seed_open=*/false));
            }
        }
        for (auto& l : spawn) legs_.push_back(std::move(l));
    }

    // Rehydrate an OPEN book from a live parent on restart (S-2026-07-05). Per-session
    // leg state is ephemeral, so we re-seed the 2 BASE legs open+armed from the parent
    // peak (any pre-restart ladder legs are lost — their banked clips already persisted
    // to the durable log; open ladder legs would have been flushed anyway). Documented
    // reset, same philosophy as the single-leg engine's seed_open.
    void seed_open(double entry_px, int64_t entry_ts_ms, double peak_px, int64_t now_ms) {
        if (!legs_.empty() || entry_px <= 0.0) return;
        const int64_t ebar = entry_ts_ms / (cfg_.tf_secs * 1000);
        const int64_t nbar = now_ms      / (cfg_.tf_secs * 1000);
        const double  peak_mfe = (peak_px > entry_px) ? (peak_px / entry_px - 1.0) * 100.0 : 0.0;
        entry_ref_ = entry_px;
        legs_.push_back(seed_leg_("T1", entry_px, cfg_.tight, entry_ts_ms, ebar, nbar, peak_mfe));
        legs_.push_back(seed_leg_("T2", entry_px, cfg_.wide,  entry_ts_ms, ebar, nbar, peak_mfe));
    }

    // ── live snapshots for the Omega desk CRYPTO COMPANIONS panel ──────────
    struct LiveSnap {
        std::string label;                // "" = book aggregate, else per-leg id
        bool   open = false, armed = false;
        double peak_mfe_pct = 0.0;
        int    bars_since_high = 0;
        int    clips = 0;                 // book-level (durable)
        double bank_bp = 0.0;             // book-level (durable)
    };
    LiveSnap snapshot() const {           // book aggregate (back-compat)
        LiveSnap s; s.clips = clip_num_; s.bank_bp = banked_bp_;
        for (const auto& lg : legs_) {
            if (!lg.open) continue;
            s.open = true;
            if (lg.mfe >= lg.arm) s.armed = true;
            if (lg.mfe > s.peak_mfe_pct) { s.peak_mfe_pct = lg.mfe; s.bars_since_high = (int)(cur_bar_ - lg.ext_bar); }
        }
        return s;
    }
    std::vector<LiveSnap> leg_snapshots() const {   // per-leg (multi-leg GUI)
        std::vector<LiveSnap> v;
        for (const auto& lg : legs_) {
            if (!lg.open) continue;
            LiveSnap s; s.label = lg.label; s.open = true;
            s.armed = (lg.mfe >= lg.arm); s.peak_mfe_pct = lg.mfe;
            s.bars_since_high = (int)(cur_bar_ - lg.ext_bar);
            v.push_back(s);
        }
        return v;
    }

private:
    void reset_session_() { legs_.clear(); entry_ref_ = 0.0; }

    void init_base_legs_(double epx, int64_t ts, int64_t bar) {
        legs_.push_back(make_leg_("T1", epx, cfg_.tight, ts, bar, false));
        legs_.push_back(make_leg_("T2", epx, cfg_.wide,  ts, bar, false));
    }

    Leg make_leg_(std::string label, double epx, const Tier& t, int64_t /*ts*/, int64_t /*bar*/, bool /*seed*/) {
        Leg l; l.label = std::move(label);
        l.epx = epx; l.le = epx; l.arm = t.arm; l.stall = t.stall; l.gb = t.gb;
        l.rc = cfg_.reclip_pct; l.cg = cfg_.cost_gate_bp;
        return l;   // open=false until first step (matches python: open set on first observation)
    }

    Leg seed_leg_(std::string label, double epx, const Tier& t,
                  int64_t ts, int64_t ebar, int64_t nbar, double peak_mfe) {
        Leg l = make_leg_(std::move(label), epx, t, ts, ebar, false);
        l.open = true; l.open_ts = ts; l.open_bar = ebar;
        l.mfe = peak_mfe; l.ext_bar = nbar;   // stall fresh from restart (peak ts not persisted)
        return l;
    }

    std::string next_ladder_label_(size_t idx_after) {
        // legs 0,1 = T1,T2 ; ladder legs = L1,L2,... (idx_after is the size at spawn)
        return "L" + std::to_string((int)idx_after - 1);
    }

    // faithful python Leg.step — returns true + gross_bp + reason on a booked clip.
    bool step_leg_(Leg& lg, int64_t bar, double cur, double& gross_out, const char*& reason_out) {
        const double fav = (cur - lg.epx) / lg.epx * 100.0;
        cur_bar_ = bar;
        if (lg.clipped) {
            if (lg.rc > 0.0 && lg.pk > 0.0 && fav > lg.pk * (1.0 + lg.rc)) {
                lg.clipped = false; lg.le = cur;      // RECLIP = re-enter at current price
            } else return false;
        }
        if (!lg.open) { lg.open = true; lg.open_ts = bar * cfg_.tf_secs * 1000; lg.open_bar = bar; lg.mfe = fav; lg.ext_bar = bar; }
        if (fav > lg.mfe + 1e-9) { lg.mfe = fav; lg.ext_bar = bar; }
        const bool armed = lg.mfe >= lg.arm;
        const int  stall = (int)(bar - lg.ext_bar);
        if (armed && lg.stall > 0 && stall >= lg.stall)                 return clip_leg_(lg, cur, gross_out, reason_out, "STALL_CLIP");
        if (armed && lg.gb > 0.0 && fav <= lg.mfe * (1.0 - lg.gb))      return clip_leg_(lg, cur, gross_out, reason_out, "REVERSAL_CLIP");
        return false;
    }

    // faithful python Leg._clip — HARD COST-COVER gate suppresses sub-cost clips.
    bool clip_leg_(Leg& lg, double cur, double& gross_out, const char*& reason_out, const char* reason) {
        const double gross = (cur / lg.le - 1.0) * 1e4;
        if (lg.cg > 0.0 && gross < lg.cg) return false;   // cost not covered -> keep holding
        lg.pk = lg.mfe; lg.clipped = true;
        gross_out = gross; reason_out = reason;
        return true;
    }

    void flush_leg_(Leg& lg, double px, int64_t ts, int64_t bar) {   // always MTM (no abandon)
        if (!lg.open || lg.clipped) return;
        const double gross = (lg.le > 0.0) ? (px / lg.le - 1.0) * 1e4 : 0.0;
        emit_clip_(lg, px, ts, bar, gross, gross - cfg_.round_trip_bp, "ENGINE_EXIT");
        lg.open = false;
    }

    void emit_clip_(Leg& lg, double exit_px, int64_t ts, int64_t bar,
                    double gross, double net, const char* reason) {
        ClipRecord r;
        r.tag = cfg_.tag + "-" + lg.label; r.symbol = cfg_.symbol; r.reason = reason;
        r.entry_ts_ms = lg.open_ts; r.exit_ts_ms = ts;
        r.entry_px = lg.le; r.exit_px = exit_px;
        r.gross_bp = gross; r.net_bp = net; r.mfe_pct = lg.mfe;
        r.bars_held = (int)(bar - lg.open_bar);
        r.clip_num = ++clip_num_;
        r.shadow = shadow_mode;
        banked_bp_ += net;
        if (on_clip_) on_clip_(r);
        std::printf("[CLIP][%s] %s net=%+.1fbp gross=%+.1fbp mfe=%.2f%% bars=%d px %.6f->%.6f shadow=%d\n",
            r.tag.c_str(), reason, net, gross, lg.mfe, r.bars_held, lg.le, exit_px, shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    Config        cfg_;
    ClipCallback  on_clip_;
    std::vector<Leg> legs_;
    double  entry_ref_ = 0.0;
    int     clip_num_  = 0;
    double  banked_bp_ = 0.0;
    int64_t cur_bar_   = 0;   // last bar seen (for snapshot bars_since_high)
};

} // namespace chimera

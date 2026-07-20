#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// MimicLadderCompanion — TIERED-2 + SELF-FUNDING LADDER clip book (S-2026-07-05b).
//
// Successor to MimicCompanionEngine (single-leg). Same STANDALONE ADDITIVE,
// observe-only, shadow contract — but every parent MIMIC trade now runs a BOOK
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
// FAITHFUL byte-exact port of crypto_mimic_tiered_ladder_sweep.py (Leg + run_trade):
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
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace chimera {

class MimicLadderCompanion {
public:
    struct Tier { double arm = 5.0; int stall = 0; double gb = 0.0; double trail_bp = 0.0; double confirm = 0.0; };  // 0 = that lever OFF; trail_bp used only in be_floor mode; confirm>0 = per-tier BE-entry stagger (bp fav from window entry), 0 = use cfg_.confirm_bp

    struct Config {
        std::string parent_tag;        // e.g. "BTC-MIMIC-H1" (the leg we observe)
        std::string tag;               // e.g. "BTC-MIMIC-CLIP" (our own ledger tag)
        std::string symbol;            // e.g. "btcusdt"
        Tier    tight;                 // base tier 1 (tight)
        Tier    wide;                  // base tier 2 (wide) — ALSO the ladder-leg params
        // STACKED BASE ARMS (S-2026-07-07 item 5, backtest/mimic_concurrent_arms_2026-07-07.txt):
        // each extra tier opens ONE MORE base leg at window entry ("S1".."Sn"). Winner =
        // roster tight+wide + {arm 2/4/6%, g50 rev-only} stacked arms + cap 8: +18,360% vs
        // +10,283% roster cap5, 8/8 coins all-6, 2x-cost robust. Ladder spawns still use `wide`.
        std::vector<Tier> extra_base;
        double  reclip_pct    = 0.05;  // re-enter when fav > prior_peak*(1+reclip_pct)
        double  loss_cut_bp   = 0.0;   // COLD-LOSS CUT (operator 2026-07-13): >0 = an UNARMED leg
                                       // (never made BE / mfe<arm) that goes -loss_cut_bp adverse is
                                       // cut HERE at a bounded loss, instead of holding to the parent-
                                       // exit flush. "if we don't make BE, why hold the loss." A real
                                       // stop (bounds the tail); 0 = OFF (legacy no-floor behaviour).
        int     cap           = 5;     // max concurrent legs (2 base + up to cap-2 ladder)
        double  cost_gate_bp  = 0.0;   // >0 = hard cost-cover clip gate (suppress sub-cost clips)
        double  confirm_bp    = 25.0;  // OPTION-B confirmed-entry: a leg stays FLAT (books nothing,
                                       // pays no cost) until fav>=confirm_bp; a never-confirmed leg
                                       // never opens (fixes the BTC -141.39bp never-positive flush).
        bool    confirm_anchor_epx = false;  // S-2026-07-17 NEVER-PRE-BE-LOSS (feedback-no-prebe-loss-ever):
                                       // when confirm_bp>0, DON'T reset le to the confirm price on open —
                                       // keep le=epx (the window/jump entry). Then a leg that opens at
                                       // epx*(1+confirm) with confirm>=RT is ALREADY floored on open
                                       // (hwm=cur>=le*(1+RT)) so its worst clip = BE (net>=0). Removes the
                                       // pre-arm window entirely => no PREBE_CUT loss. Requires confirm_bp>RT.
        bool    anchored_reclip = false;  // S-2026-07-17 ANCHORED RECLIP (feedback-befloor-on-open-foundation
                                       // pillar 4: "reclip_pct=0 OR anchored-reclip"). With mimic_floor +
                                       // confirm_anchor_epx + reclip_pct>0, route the reclip RE-ENTRY through the
                                       // SAME confirm+anchor path as the initial open instead of the raw le=cur
                                       // reclip: on a floored clip the leg is reset FLAT with a NEW anchor epx = the
                                       // exit px, and the existing confirm gate re-opens it only once fav(from the new
                                       // epx) >= confirm, with le=epx (confirm_anchor_epx) => floored ON OPEN =>
                                       // net_bp_real >= 0 by construction (no pre-arm window). Recovers reclip-driven
                                       // edge (THETA/DOGE/TRX lost it when reclip was forced to 0 for compliance)
                                       // WITHOUT the raw-reclip pre-BE leak (ADA -527bp, TRX -1773bp). REQUIRES
                                       // confirm_anchor_epx=true — otherwise the re-open sets le=cur and reopens the
                                       // leak; enforced in book_mimic_stop_ (falls back to reclip-off behaviour if not).
        int64_t tf_secs       = 3600;  // H1
        double  round_trip_bp = 20.0;  // 0.20% RT Binance spot taker
        // ── BE-FLOOR mode (S-2026-07-05 resume, operator restated spec) ──────────
        // When true the leg exits are governed by a HARD BREAK-EVEN FLOOR instead of
        // giveback/stall + MTM flush. Faithful port of be_bptrail.py leg_book:
        //   • a leg stays FLAT until price clears +be_bp from its ref (== RT cost);
        //     it OPENS at that price (le), so net starts at 0 (the move paid the cost).
        //   • stop = max(le, hwm*(1 - trail_bp/1e4)); exit the instant cur<=stop.
        //     exit_px = stop >= le ALWAYS -> gross>=0 -> net = gross (NO 2nd RT charge,
        //     the +be_bp open move already paid it). NO underwater flush, EVER.
        //   • reclip: after an exit the ref becomes the exit price; the leg re-opens on
        //     the next +be_bp continuation. NO self-funding ladder (operator dropped it).
        // Result: net_bp >= 0 on EVERY clip BY CONSTRUCTION. Trigger (W/thr) lives in
        // the dedicated shadow detector engine the companion observes — parent untouched.
        bool    be_floor      = false;
        double  be_bp         = 20.0;  // gross bp move from ref required to open a leg (== cost)
        // Internal mimic detector (BOTH modes since S-2026-07-07w; was be_floor-only).
        // >0 = the companion self-detects its OWN long-event window from the price stream
        // it already receives — it does NOT read the live parent's position (independence:
        // the parent is never retuned for the companion's sake). Faithful to sw.parent(W,thr)
        // on H1 closes; enter when close/close[-W]-1 >= det_thr, exit when <= -det_thr.
        // LADDER mode uses the roster per-coin W/thr (the +18,360% winner was swept on those
        // windows — the live parents are UNIFORM 4h/+2% since 52c0d31 and must NOT be the
        // companion's trigger: different window family than the backtest). W-hour cold-start
        // on restart (no warm-seed; per-trade legs are ephemeral).
        int     det_w         = 0;
        double  det_thr       = 0.0;
        // ── S-2026-07-18 BOUNDED CATCH-UP (operator: "do the bounded thing") ─────
        // A restart/outage over a qualifying jump used to lose the window forever:
        // seed_det_ring_hist() refills the ring but leaves the detector FLAT, so a
        // book that was down when the jump printed never arms (INJ 2026-07-18: +1.76%
        // 06:00Z jump predated the 22:57Z zeroing restart -> no window, no trades).
        // >0 = at warm-seed, RE-OPEN the detector window from a historical jump iff
        // ALL of (certified catchup_outage_bt.cpp; equivalence-to-always-on class):
        //   • the replayed detector would still be IN the window (no j<=-thr since);
        //   • the jump is at most catchup_max_age_bars finalized bars old (the BOUND);
        //   • NO close since the jump reached epx*(1+min tier confirm) — i.e. an
        //     always-on book would still be FLAT waiting for confirm. If confirm was
        //     crossed during the outage the window is SKIPPED (back-filling that open
        //     = the forbidden late-chase / backdated-entry class, S-2026-07-08);
        //   • confirmed-entry mimic family only (mimic_floor, min confirm > 0; never
        //     jump_floor — its immediate entry at NOW price would chase — never the
        //     retired be_floor family) and arming_allowed_() (retire/rank_out hold).
        // Catch-up sets det_in_/det_entry_ ONLY. Legs init + open via the untouched
        // live confirm path (BE-ENTRY, le=epx anchor, floored-on-open) — the entry a
        // catch-up window books is the SAME confirm-cross an always-on book would
        // have booked. 0 = OFF.
        int     catchup_max_age_bars = 0;
        // ── JUMP-FLOOR mode (S-2026-07-14, operator: "add these to our trades") ──
        // Third mode, faithful to Crypto/backtest/mimic2pct_be_bt.cpp `percoin`
        // winning cells (17/19 plateau-validated per-coin lever map). EXPLICIT
        // operator override of the immediate-entry ban for THESE backtested cells.
        // Distinct from BOTH other modes: NOT the retired be_floor family (which
        // waits for +be_bp before opening) and NOT the confirmed-entry mimics.
        //   • ENTER immediately at the first tick after a detect close (== BT
        //     "next open") when close/close[-W]-1 >= det_thr. Pays cost; CAN lose
        //     pre-BE — that exposure is in the BT verdict (worst column shown).
        //   • optional pre-BE hard stop jf_prebe_stop_bp below entry (intrabar).
        //   • BE-FLOOR: once a CLOSE covers entry*(1+RT), stop >= entry*(1+RT) —
        //     after arming the trade cannot close negative (barring gap-through).
        //   • trail (jf_giveback<1): stop = max(floor, E*(1+mfe*(1-g))), close-
        //     based mfe, intrabar fill. g=1.0 disables trail = ride-to-reversal.
        //   • REVERSAL: j <= -det_thr at close -> exit at next tick (BT next open).
        //   • re-entry requires a fresh jump: a close with j < det_thr must print
        //     after ANY exit before the next entry may arm (BT `armed` flag).
        // ADVERSE-PROTECTION: backtested verdict per cell — BE-floor yes, pre-BE
        // hard stop harmful except LINK (s=2%); retire_bp = -2x the cell's BT maxDD.
        bool    jump_floor       = false;
        double  jf_giveback      = 1.0;   // g: fraction of MFE given back before trail exit; 1.0 = reversal-only
        double  jf_prebe_stop_bp = 0.0;   // s: pre-BE hard stop bp below entry; 0 = none
        // ── S-2026-07-17s SIGNAL-ONLY (operator: "i want no more mimic") ────────
        // The full jf state machine still runs (detector + entry + floor + trail) so a
        // companion-on-companion mimic can observe jf_in_position()/jf_entry_px(), but
        // the cell BOOKS NOTHING: no bank, no retire check; the ClipRecord is stamped
        // signal_only=true so the on_clip_ handler skips the ledger/desk (it still runs
        // for the Fig-2 immediate det-state persist — resurrection defense unchanged).
        // Used by GRT-PJ5W1: the imm-entry mimic BOOK is dead, only its signal drives
        // the certified GRT-PJ5W1-MIM floored mimic.
        bool    signal_only      = false;
        // ── MIMIC-FLOOR mode (S-2026-07-15, operator: unify EVERY mimic/companion cell
        // to ONE exit = BE-floored tight-giveback trail — the honest jump_floor floor+HWM
        // trail math ported onto the CONFIRMED-ENTRY ladder path so all SWEET/REGIME cells
        // share it). When true, an OPEN leg is managed by this instead of the floorless
        // Tier.gb / stall exits:
        //   • hwm tracks the leg high since its fill (le); floor arms once a mark covers
        //     le*(1+RT) (the leg pays its OWN round-trip — NOT the retired be_floor "move
        //     already paid it" fiction; this books the honest worse-of fill).
        //   • per-tick trail stop = max(le*(1+RT), le*(1+(hwm/le-1)*(1-g))), checked INTRA-
        //     BAR (stop_check path / live per tick), booked at the stop -> after arming a
        //     leg CANNOT close below BE (net_bp_real >= 0, jump_floor parity). Fixes the
        //     H1-close giveback RACE (INJ T1 -96.8bp) + the floorless negative-clip gap.
        //   • g = mimic_giveback: fraction of the run above le given back before the trail
        //     exits. 1.0 = ride to window-exit (still floored). smaller g = tighter/earlier.
        //   • entry/confirm/reclip/window structure unchanged; loss_cut still bounds the
        //     PRE-arm tail (a leg that never covers its cost). hwm gauged from le (the fill),
        //     not epx, so the floor guarantees the LEG's own book, not the detector's.
        bool    mimic_floor    = false;
        double  mimic_giveback = 1.0;
        // ── S-2026-07-16 STAGGERED FLOORED LADDER (operator: "once we get to BE ... open at
        // least 4x mimics, stagger them ... all protections ... never go negative on a mimic") ──
        // When mimic_floor && mimic_stagger: init builds T1 + T2 + extra_base as SEPARATE legs,
        // each with its own per-tier `confirm` (escalating BE-entry: e.g. BE / +1% / +2% / +3%).
        // Each leg floors at its OWN fill (le) => the legs are DISTINCT positions (different
        // entries), NOT the redundant same-entry N-multiply the single-leg block guards against,
        // so they are genuinely additive (07-07 concurrent-arms: staggered legs additive) AND
        // each is post-arm BE-floored (never books negative). All legs eligible (confirm gates
        // the opens, not advance_stagger_); set cap == #tiers so no self-funding ladder spawns
        // beyond the staggered set. mimic_giveback stays uniform (g-sweep: tightening HURTS).
        bool    mimic_stagger  = false;
        // ── S-2026-07-08 WEIGHTING + AUTO-RETIREMENT (Crypto backtest/mimic_weighting_bt.cpp) ──
        // size_mult: per-coin notional weight (x2 robust top performer / x1 baseline).
        //   Stamped on every ClipRecord; weighted bank = Σ net_bp_real * size_mult
        //   (bank_bp_real_w). The RAW real column is untouched — retirement + parity
        //   stay in unweighted per-leg bp.
        double  size_mult     = 1.0;
        // rank_out: book takes NO NEW windows (detector enter + base-leg init suppressed);
        //   open legs manage + flush normally; durable state preserved. For per-coin
        //   BT-net-negative books (none as of 2026-07-08 — machinery wired for the
        //   monthly re-test to use).
        bool    rank_out      = false;
        // retire_bp: AUTO-RETIREMENT threshold on the book's banked REAL net (raw bp,
        //   restored at boot from its own clip ledger). <0 enables: when
        //   banked_bp_real <= retire_bp the book RETIRES — stops arming new windows,
        //   open legs manage normally, loud one-shot [CLIP-RETIRE] log. Default per
        //   coin = -2x the worst validated per-book BT drawdown episode (a level
        //   beyond anything the validated backtest ever produced). 0 = off.
        double  retire_bp     = 0.0;
        // retire_override: operator un-retire (deliberate act): tag listed in
        //   data/companion_unretire.flags -> retirement latch disabled this run.
        bool    retire_override = false;
        // ── STAGGERED-OPEN (S-2026-07-11, operator full-roster) ──────────────────
        // Controls WHEN each base tier leg is allowed to OPEN, to cut the worst-case
        // simultaneous drawdown (all legs opening at the window-entry top and dropping
        // together). Independent of arm/gb/stall (those still govern each leg once open).
        // Base legs open in tier order (T1,T2,S1,S2,...); a leg with eligible=false is
        // held FLAT (does not open) until advance_stagger_ releases it. Long-only, shadow,
        // parent untouched. 0 = OFF (every existing cell unchanged — legs all eligible).
        //   1 = BE_CASCADE: release the next leg only once EVERY already-released leg is
        //       open AND break-even (mfe >= stagger_be_bp). => at most ONE un-BE'd leg at
        //       a time. Requires reclip OFF + cap==#base (no ladder) to hold the guarantee.
        //   2 = TIME: release the next leg every stagger_k bars after the previous release.
        int     stagger_mode  = 0;
        int     stagger_k     = 0;      // TIME: bars between successive leg releases
        double  stagger_be_bp = 20.0;   // BE_CASCADE: mfe(bp) for a released leg to count "BE'd"
    };

    // Emitted on every clip / engine-exit. main.cpp persists to the companion ledger.
    struct ClipRecord {
        std::string tag, symbol, reason;   // STALL_CLIP / REVERSAL_CLIP / ENGINE_EXIT
        int64_t entry_ts_ms = 0, exit_ts_ms = 0;
        double  entry_px = 0.0, exit_px = 0.0;
        double  gross_bp = 0.0, net_bp = 0.0, mfe_pct = 0.0;
        // S-2026-07-07f HONEST REAL COLUMN (Omega befloor-family real-fill audit): the be_floor
        // model column books fill-at-floor + max(0,.) + net=gross (no cost) — an accounting
        // tautology (neg=0 by construction), proven -1.13M bp real vs +3.6M bp model on 2021-26
        // Binance H1 across the 10-coin roster. These fields carry the worse-of fill (the H1
        // close that tripped the stop) minus the 20bp RT cost. Judge the book on THESE.
        double  gross_bp_real = 0.0, net_bp_real = 0.0;
        double  size_mult = 1.0;           // per-coin weight this clip was booked under (S-2026-07-08)
        int     bars_held = 0, clip_num = 0;
        bool    shadow = true;
        bool    signal_only = false;       // S-17s: handler must NOT ledger/desk this record
    };
    using ClipCallback = std::function<void(const ClipRecord&)>;

    // ── S-2026-07-18 LIVE-MIRROR OPEN EVENT (operator go-live order: ALL crypto live) ──
    // Fired when a leg transitions FLAT->OPEN on a LIVE tick/close path only — NOT on
    // restart-restore / warm-seed / rehydrate (those re-attach to a notional that was
    // already open; announcing them would double-buy). main.cpp's LiveMimicMirror
    // subscribes and routes a clamped real order through the ONE ExecutionGateway
    // (PILOT-SCOPE symbol/order/gross caps apply there). Pure observer: the companion
    // book is UNTOUCHED and reads nothing back (independent-engine rule). tag mirrors
    // emit_clip_ (cfg_.tag + "-" + label) so the eventual ClipRecord of the SAME leg
    // carries the SAME tag — the mirror pairs its sell to the buy by that key.
    // signal_only cells never announce (they book nothing and must not trade).
    struct OpenRecord {
        std::string tag, symbol, label;
        int64_t ts_ms = 0;
        double  px    = 0.0;   // the CURRENT mark the live mirror would buy at
        // S-2026-07-18s MIRROR ACQUIRE GATE (LiveMirrorIncident20260718 fix 3): the leg's
        // anchored BE reference (le — for confirm_anchor_epx legs the window-entry anchor).
        // The mirror REFUSES a BUY whose px sits above floor_anchor*(1+cost_bound): a
        // restored/late leg must never chase a fill the shadow floor can't protect
        // (UNI: shadow floor 3.563, live fill 3.640 = -2.2% naked under the floor).
        double  floor_anchor = 0.0;
    };
    using OpenCallback = std::function<void(const OpenRecord&)>;
    void set_on_leg_open(OpenCallback cb) { on_leg_open_ = std::move(cb); }

    explicit MimicLadderCompanion(Config c) : cfg_(std::move(c)) {}

    void set_on_clip(ClipCallback cb) { on_clip_ = std::move(cb); }
    void set_rank_out(bool v) { cfg_.rank_out = v; }   // MIMIC-FLOOR-GATE: refuse-to-arm (no new windows)
    const Config& config() const { return cfg_; }
    bool  is_open() const { for (auto& l : legs_) if (l.open) return true; return false; }
    int   clips()   const { return clip_num_; }
    void  rehydrate(int clips_total, double bank_bp_total, double bank_bp_real_total = 0.0,
                    double bank_bp_real_w_total = 0.0) {
        clip_num_ = clips_total; banked_bp_ = bank_bp_total; banked_bp_real_ = bank_bp_real_total;
        banked_bp_real_w_ = bank_bp_real_w_total;
        check_retire_();   // restore-at-boot: forward bank already through threshold -> retire now
    }
    bool  is_retired() const { return retired_; }
    bool  shadow_mode = true;

    // ── S-2026-07-08 detector-state persistence (restart-path fix) ──────────
    // The old restart path seeded the roster-W/thr detector book from the LIVE
    // PARENT's position — but the parents run the UNIFORM 4h/+2% window (52c0d31),
    // a DIFFERENT window family (never conflate, feedback-test-operator-spec).
    // A restart therefore either injected a window the roster detector never
    // opened (phantom arms) or ate a genuine in-flight detector window (lost
    // arms). Fix: persist the detector's own state each H1 close and restore it
    // verbatim at boot; legs re-open FLAT through the gated step path
    // (rehydrate-FLAT, no backdated le).
    std::string det_state_json() const {
        std::ostringstream js; js << std::fixed << std::setprecision(8);
        js << "{\"tag\":\"" << cfg_.tag << "\",\"det_in\":" << (det_in_ ? 1 : 0)
           << ",\"det_entry\":" << det_entry_ << ",\"det_bar\":" << det_bar_
           << ",\"det_close\":" << det_close_
           // S-2026-07-17 FLOOR-RATCHET PERSISTENCE (ETH-PJ7W24 zombie incident): the floor
           // state itself is persisted so a restart can NEVER disarm an armed floor. The old
           // restore recomputed floored from the last persisted close alone — a close that had
           // dipped below entry*(1+rt) resurrected an ARMED leg as UN-floored with the raw
           // pre-BE stop (-400bp exposure on a leg whose floor had already armed).
           << ",\"det_floored\":" << (jf_floored_ ? 1 : 0)
           << ",\"det_mfe\":" << jf_mfe_ << ",\"h1c\":[";
        for (size_t i = 0; i < h1c_.size(); ++i) { if (i) js << ","; js << h1c_[i]; }
        js << "]}";
        return js.str();
    }
    void restore_det_state(bool in, double entry, int64_t bar, double close,
                           bool floored_p, double mfe_p,
                           const std::vector<double>& h1c, int64_t now_ms) {
        if (cfg_.det_w <= 0) return;
        h1c_ = h1c;
        if ((int)h1c_.size() > cfg_.det_w + 1)
            h1c_.erase(h1c_.begin(), h1c_.end() - (cfg_.det_w + 1));
        det_bar_ = bar; det_close_ = close;
        det_in_ = in; det_entry_ = entry;
        if (cfg_.jump_floor) {
            // JUMP-FLOOR restore: rebuild jf state from the mirrored det fields.
            // S-2026-07-17 FLOOR RATCHET HONORED (ETH-PJ7W24 zombie incident): floored/mfe are
            // now persisted and restored verbatim — once a floor has armed it can NEVER disarm
            // across a restart. The old path recomputed floored from the last close alone; a
            // close below entry*(1+rt) restored an armed leg UN-floored with the raw pre-BE
            // stop. mfe = max(persisted hwm, last close) so the trail never loosens either.
            // Flat books restore UN-armed: a j<thr close must print before the next entry
            // (conservative fresh-jump rule).
            jf_in_ = det_in_ && det_entry_ > 0.0;
            jf_armed_ = false;
            if (jf_in_) {
                const double rt = cfg_.round_trip_bp * 1e-4;
                jf_E_ = det_entry_;
                jf_mfe_ = std::max({0.0, mfe_p, (close > 0.0 ? close / jf_E_ - 1.0 : 0.0)});
                jf_floored_ = floored_p || jf_mfe_ >= rt;
                jf_stop_ = (cfg_.jf_prebe_stop_bp > 0.0) ? jf_E_ * (1.0 - cfg_.jf_prebe_stop_bp / 1e4) : -1.0;
                if (jf_floored_) {
                    double ns = jf_E_ * (1.0 + rt);
                    if (cfg_.jf_giveback < 1.0)
                        ns = std::max(ns, jf_E_ * (1.0 + jf_mfe_ * (1.0 - cfg_.jf_giveback)));
                    jf_stop_ = std::max(jf_stop_, ns);
                }
                jf_open_ts_ = now_ms; jf_open_bar_ = bar; jf_ext_bar_ = bar;   // held-bars restart at boot (open ts not persisted)
                legs_.clear();
                Leg l; l.label = "J1"; l.epx = jf_E_; l.le = jf_E_; l.arm = cfg_.round_trip_bp / 100.0;
                l.open = true; l.open_ts = now_ms; l.open_bar = bar; l.ext_bar = bar;
                l.mfe = jf_mfe_ * 100.0;
                legs_.push_back(l);
                jf_restored_ = true;     // first live tick must honor the floor, not a downtime-gap px
            }
            std::printf("[CLIP-DETSEED] %s JUMPFLOOR in=%d entry=%.6f floored=%d stop=%.6f ring=%zu\n",
                cfg_.tag.c_str(), jf_in_ ? 1 : 0, jf_E_, jf_floored_ ? 1 : 0, jf_stop_, h1c_.size());
            return;
        }
        if (det_in_ && det_entry_ > 0.0 && legs_.empty() && !cfg_.be_floor && arming_allowed_()) {
            entry_ref_ = det_entry_;
            cur_bar_   = now_ms / (cfg_.tf_secs * 1000);
            init_base_legs_(det_entry_, now_ms, cur_bar_);
            for (auto& lg : legs_) lg.seeded_flat = true;   // le anchors at first live mark
        }
        std::printf("[CLIP-DETSEED] %s det_in=%d entry=%.6f ring=%zu (own-window restore, parent NOT used)\n",
            cfg_.tag.c_str(), det_in_ ? 1 : 0, det_entry_, h1c_.size());
    }

    // ── S-2026-07-14 warm-seed: detector ring from historical H1 closes ─────
    // restore_det_state() only covers RESTARTS (persisted ring). A BRAND-NEW
    // det_w cell has nothing persisted, so its W-bar ring sat cold for det_w
    // hours after first deploy (ETH-PJ7W24 ≈ 25h blind — operator: NEVER wait
    // for warmup). This fills the ring from REST klines at boot; also refreshes
    // a ring whose persisted state went stale across an outage (det_bar_ behind
    // the last closed kline). STATE ONLY — faithful to a book that ran all along:
    //   • closes[] = FINALIZED H1 bars oldest-first. The LAST one loads as the
    //     pending (det_bar_/det_close_) bar, so the normal live path finalizes
    //     it and can ENTER on it at the next real tick — real fill px, never a
    //     backdated entry (the S-2026-07-08 forbidden class).
    //   • jump_floor arm state replayed over the closes (historical would-enter
    //     events consume the arm; fresh-jump rule preserved exactly).
    //   • a restored IN-FLIGHT window is untouched (ring refresh only).
    //   • ladder/be_floor books get the ring only, detector stays flat: first
    //     LIVE close evaluates with a full window instead of waiting W bars.
    int det_ring_size() const { return (int)h1c_.size(); }
    // min per-tier confirm (bp) across the base tiers; 0 => some tier opens unconfirmed.
    double min_confirm_bp_() const {
        double m = (cfg_.tight.confirm > 0.0 ? cfg_.tight.confirm : cfg_.confirm_bp);
        const double w = (cfg_.wide.confirm > 0.0 ? cfg_.wide.confirm : cfg_.confirm_bp);
        if (w < m) m = w;
        for (const auto& t : cfg_.extra_base) {
            const double e = (t.confirm > 0.0 ? t.confirm : cfg_.confirm_bp);
            if (e < m) m = e;
        }
        return m;
    }
    void seed_det_ring_hist(const std::vector<double>& closes, int64_t last_closed_bar) {
        if (cfg_.det_w <= 0 || closes.size() < 2) return;
        const bool warm = (int)h1c_.size() >= cfg_.det_w + 1;
        if (warm && det_bar_ >= last_closed_bar) return;          // persisted state current — nothing to do
        const bool in_flight = det_in_ || jf_in_;
        h1c_.clear();
        bool armed = false;
        // BOUNDED CATCH-UP replay state (see Config::catchup_max_age_bars): walk the same
        // finalized closes with the LIVE detector rule (enter j>=thr, exit j<=-thr) so we
        // know whether an always-on book would still be in a window right now.
        const double minconf = min_confirm_bp_();
        const bool catchup_eligible = cfg_.catchup_max_age_bars > 0 && !in_flight
            && cfg_.mimic_floor && !cfg_.jump_floor && !cfg_.be_floor
            && minconf > 0.0 && arming_allowed_();
        bool   cu_in = false; double cu_entry = 0.0, cu_maxafter = 0.0;
        long   cu_entry_i = -1, fin_i = -1;                       // finalized-close indices
        for (size_t i = 0; i + 1 < closes.size(); ++i) {          // all but last -> finalized ring
            if (closes[i] <= 0.0) continue;
            h1c_.push_back(closes[i]);
            if ((int)h1c_.size() > cfg_.det_w + 1) h1c_.erase(h1c_.begin());
            ++fin_i;
            if ((int)h1c_.size() >= cfg_.det_w + 1) {
                const double j = h1c_.back() / h1c_.front() - 1.0;
                if (!in_flight && cfg_.jump_floor) {
                    if (!armed) { if (j < cfg_.det_thr) armed = true; }
                    else if (j >= cfg_.det_thr) armed = false;    // would-enter historically: arm consumed, need fresh re-arm
                }
                if (catchup_eligible) {
                    if (!cu_in && j >= cfg_.det_thr) { cu_in = true; cu_entry = closes[i]; cu_entry_i = fin_i; cu_maxafter = closes[i]; }
                    else if (cu_in && j <= -cfg_.det_thr) { cu_in = false; cu_entry = 0.0; cu_entry_i = -1; }
                }
            }
            if (cu_in && closes[i] > cu_maxafter) cu_maxafter = closes[i];
        }
        det_bar_ = last_closed_bar; det_close_ = closes.back();   // pending bar: live path finalizes it (entry only on a live tick)
        if (!in_flight && cfg_.jump_floor) jf_armed_ = armed;
        // BOUNDED CATCH-UP verdict: re-open the window ONLY if the always-on book would
        // still be flat-in-window (confirm never crossed, pending close included) and the
        // jump is inside the bound. det_in_/det_entry_ only — legs init + open through the
        // normal live confirm path (floored-on-open), never a backdated fill.
        bool cu_armed = false;
        if (catchup_eligible && cu_in && cu_entry > 0.0) {
            const long   age = fin_i - cu_entry_i;
            const double conf_px = cu_entry * (1.0 + minconf / 1e4);
            const double pend = closes.back();
            if (age <= (long)cfg_.catchup_max_age_bars && cu_maxafter < conf_px
                && (pend <= 0.0 || pend < conf_px)) {
                det_in_ = true; det_entry_ = cu_entry; cu_armed = true;
                std::printf("[CLIP-CATCHUP] %s window re-opened from history: entry=%.6f age=%ldb (bound %d) "
                    "maxafter=%.6f < confirm_px=%.6f (minconf=%.0fbp) — legs open only via live confirm path\n",
                    cfg_.tag.c_str(), cu_entry, age, cfg_.catchup_max_age_bars, cu_maxafter, conf_px, minconf);
            }
        }
        std::printf("[CLIP-SEED] %s det ring warm-seeded from history: %zu closes ring=%zu W=%d armed=%d in_flight=%d catchup=%d\n",
            cfg_.tag.c_str(), closes.size(), h1c_.size(), cfg_.det_w,
            (cfg_.jump_floor && jf_armed_) ? 1 : 0, in_flight ? 1 : 0, cu_armed ? 1 : 0);
    }

    // ── one independent clip leg (faithful python Leg) ─────────────────────
    struct Leg {
        std::string label;      // "T1" / "T2" / "L1".. (tier / ladder id for the GUI)
        double  epx = 0.0;      // FIXED entry — fav/mfe/arm/reclip gauge
        double  le  = 0.0;      // MOVING leg entry — clip gross gauge (resets on reclip)
        double  arm = 5.0; int stall = 0; double gb = 0.0; double rc = 0.05; double cg = 0.0; double confirm = 0.0;
        bool    open = false, clipped = false;
        bool    seeded_flat = false;   // restart-rehydrated leg: le anchors at first live mark on open
        bool    eligible = true;       // STAGGER gate: false = held FLAT until advance_stagger_ releases it
        int     tier_idx = 0;          // base-tier open order (T1=0,T2=1,S1=2,...) for staggered release
        double  pk = 0.0, mfe = 0.0;
        int64_t ext_bar = 0, open_bar = 0, open_ts = 0;
        // BE-floor / MIMIC-FLOOR mode state
        double  trail_bp = 0.0;   // bp giveback-from-hwm (0 = ride to parent exit)
        double  hwm = 0.0;        // high-water price since open
        double  ref_px = 0.0;     // reference the +be_bp open gate is measured from (resets to exit px on reclip)
        bool    floored = false;  // MIMIC-FLOOR: a mark has covered le*(1+RT) -> post-arm exit can't go negative
        double  stop_px = -1.0;   // MIMIC-FLOOR: resting trail stop px (ratcheted on close, checked intrabar); <0 = none
    };

    // Drive ONCE per completed parent bar (byte-exact vs python) OR per tick
    // (intra-bar; bar index = ts/H1 only advances hourly so STALL stays H1-quantised
    // and REVERSAL/RECLIP price gates fire the instant they trip). Reads the parent's
    // settled position only, never writes to it. Long-only (MIMIC is always long).
    void observe(bool parent_in_pos, double parent_entry_px, double cur_px, int64_t ts_ms) {
        const int64_t bar = ts_ms / (cfg_.tf_secs * 1000);
        if (cfg_.det_w > 0) {
            feed_selfdetect_(cur_px, ts_ms);
            if (cfg_.jump_floor) { jf_on_tick_(cur_px, ts_ms); return; }               // JUMP-FLOOR: fills + intrabar stops
            if (cfg_.loss_cut_bp > 0.0) intrabar_reversal_cut_(cur_px, ts_ms, bar);   // per-tick HARD STOP
            if (cfg_.mimic_floor)       intrabar_mimic_floor_(cur_px, ts_ms, bar);    // per-tick BE-floored trail stop
            return;                                                                    // internal detector (live, both modes)
        }
        if (cfg_.loss_cut_bp > 0.0) intrabar_reversal_cut_(cur_px, ts_ms, bar);       // per-tick HARD STOP (det_w==0 path too)
        if (cfg_.mimic_floor)       intrabar_mimic_floor_(cur_px, ts_ms, bar);        // per-tick BE-floored trail (det_w==0 too)
        if (cfg_.be_floor) {
            observe_be_(parent_in_pos, parent_entry_px, cur_px, ts_ms, bar);         // external window (validation harness)
            return;
        }
        observe_ladder_(parent_in_pos, parent_entry_px, cur_px, ts_ms, bar);         // external window (parent-driven)
    }
    // Stop-ONLY tick: runs the hard reversal cut against px WITHOUT feeding the detector
    // (backtest feeds the bar LOW here; live, observe() already runs the cut per tick).
    void stop_check_only(double px, int64_t ts_ms) {
        const int64_t bar = ts_ms / (cfg_.tf_secs * 1000);
        if (cfg_.loss_cut_bp > 0.0) intrabar_reversal_cut_(px, ts_ms, bar);
        if (cfg_.mimic_floor)       intrabar_mimic_floor_(px, ts_ms, bar);   // BE-floored trail: bar LOW tests the stop
    }

    // ── LADDER mode window driver (the faithful python run_trade walk). in_pos/entry_px
    //   come from EITHER the live parent (det_w==0 back-compat) or the internal detector. ──
    void observe_ladder_(bool parent_in_pos, double parent_entry_px, double cur_px, int64_t ts_ms, int64_t bar) {
        // Window over / no valid mark -> flush every open leg MTM, then reset.
        if (!parent_in_pos || parent_entry_px <= 0.0 || cur_px <= 0.0) {
            const double px = (cur_px > 0.0) ? cur_px : entry_ref_;
            for (auto& lg : legs_) flush_leg_(lg, px, ts_ms, bar);
            reset_session_();
            return;
        }

        // New parent trade -> flush any stragglers, reset, seed the 2 base legs.
        // RANK-OUT / RETIRED: the window is tracked (entry_ref_) but NO legs are
        // opened — "no new windows, state preserved"; open legs from before the
        // latch keep managing via the loop below.
        if (entry_ref_ != parent_entry_px) {
            for (auto& lg : legs_) flush_leg_(lg, cur_px, ts_ms, bar);
            reset_session_();
            entry_ref_ = parent_entry_px;
            if (arming_allowed_()) init_base_legs_(parent_entry_px, ts_ms, bar);
        }

        // STAGGER: release the next base leg if its gate (BE-cascade / time) is met.
        advance_stagger_(bar);

        // Step every leg; ladder-spawn on cost-covered clips (newborns added AFTER
        // the loop so they do not step the bar they are born — matches python).
        std::vector<Leg> spawn;
        for (auto& lg : legs_) {
            double gross; const char* reason;
            if (step_leg_(lg, bar, cur_px, gross, reason)) {
                const double net = gross - cfg_.round_trip_bp;
                emit_clip_(lg, cur_px, ts_ms, bar, gross, net, reason);
                if (net > 0.0 && arming_allowed_() && (int)(legs_.size() + spawn.size()) < cfg_.cap)
                    spawn.push_back(make_leg_(next_ladder_label_(legs_.size() + spawn.size()),
                                              cur_px, cfg_.wide, ts_ms, bar, /*seed_open=*/false));
            }
        }
        for (auto& l : spawn) legs_.push_back(std::move(l));
    }

    // Rehydrate a book from a live parent on restart — REHYDRATE-FLAT (S-2026-07-07w).
    // The old version re-seeded legs OPEN at the parent entry with le/floor BACKDATED —
    // through a mid-event move that books fake fills on the next flush (the ADA -244bp
    // seed_open artifact: legs re-opened OPEN at 0.186050 mid-event bypassing the +be_bp
    // gate; item-3 audit Crypto/backtest/latearm/LATEARM_VERDICT.md). Now: restore the
    // WINDOW ONLY (entry_ref_ + detector event); every leg re-opens FLAT through its
    // normal gated step path, and its le anchors at the first live mark it actually
    // opens on (seeded_flat) — no backdated floor, no phantom PnL.
    void seed_open(double entry_px, int64_t entry_ts_ms, double peak_px, int64_t now_ms) {
        if (!legs_.empty() || entry_px <= 0.0) return;
        if (!arming_allowed_()) return;   // ranked-out / retired: no new legs at rehydrate either
        (void)peak_px;
        // S-2026-07-08 RESTART-PATH FIX: a det_w book must NEVER be seeded from the
        // live PARENT — the parents run the UNIFORM 4h/+2% window (52c0d31) while this
        // book's trigger is the roster per-coin W/thr detector: a DIFFERENT window
        // family (never conflate). The old behaviour injected phantom windows on every
        // restart while a parent happened to be in-pos (and its exit then hung on a
        // cold detector ring). Rehydrate for det_w books = restore_det_state() from
        // data/companion_det_state.json; if that file is absent the book cold-starts
        // its W-hour window honestly.
        if (cfg_.det_w > 0) {
            std::printf("[CLIP-SEED-SKIP] %s det_w=%d: parent-window seed refused (wrong window "
                        "family); use restore_det_state()\n", cfg_.tag.c_str(), cfg_.det_w);
            std::fflush(stdout);
            return;
        }
        const int64_t ebar = entry_ts_ms / (cfg_.tf_secs * 1000);
        const int64_t nbar = now_ms      / (cfg_.tf_secs * 1000);
        entry_ref_ = entry_px;
        cur_bar_   = nbar;   // anchor "now" so the first post-rehydrate snapshot() before any
                             // step reports bars_since_high >= 0 (the -495347 render bug)
        if (cfg_.be_floor) {
            legs_.push_back(make_be_leg_("T1", entry_px, cfg_.tight));   // FLAT: opens on +be_bp from ref
            legs_.push_back(make_be_leg_("T2", entry_px, cfg_.wide));
        } else {
            init_base_legs_(entry_px, entry_ts_ms, ebar);                // FLAT: open on first step
            for (auto& lg : legs_) lg.seeded_flat = true;                // le anchors at first live mark
        }
    }

    // ── live snapshots for the Omega desk CRYPTO COMPANIONS panel ──────────
    struct LiveSnap {
        std::string label;                // "" = book aggregate, else per-leg id
        bool   open = false, armed = false;
        double peak_mfe_pct = 0.0;
        int    bars_since_high = 0;
        int    clips = 0;                 // book-level (durable)
        double bank_bp = 0.0;             // book-level (durable) — MODEL column (reference only)
        double bank_bp_real = 0.0;        // book-level (durable) — HONEST real-fill column (fold THIS)
        double bank_bp_real_w = 0.0;      // weighted real bank (Σ net_bp_real * size_mult) — S-2026-07-08
        double size_mult = 1.0;           // per-coin weight (S-2026-07-08)
        bool   retired = false;           // auto-retirement latch (S-2026-07-08)
    };
    // ── jump-floor parent-state readouts (read-only; backtest parent-replay
    //    harness reads the REAL jf window to drive an independent mimic leg —
    //    real_parent_mimic_bt.cpp / feedback-verify-kill-replicates-mechanism) ──
    bool   jf_in_position() const { return jf_in_; }
    double jf_entry_px()    const { return jf_E_;  }
    bool   jf_is_floored()  const { return jf_floored_; }   // S-2026-07-17: restore-gate tripwire reads this

    LiveSnap snapshot() const {           // book aggregate (back-compat)
        LiveSnap s; s.clips = clip_num_; s.bank_bp = banked_bp_; s.bank_bp_real = banked_bp_real_;
        s.bank_bp_real_w = banked_bp_real_w_; s.size_mult = cfg_.size_mult;
        s.retired = retired_ || cfg_.rank_out;
        for (const auto& lg : legs_) {
            if (!lg.open) continue;
            s.open = true;
            if (lg.mfe >= lg.arm) s.armed = true;
            if (lg.mfe > s.peak_mfe_pct) { s.peak_mfe_pct = lg.mfe; s.bars_since_high = (int)std::max<int64_t>(0, cur_bar_ - lg.ext_bar); }
        }
        return s;
    }
    std::vector<LiveSnap> leg_snapshots() const {   // per-leg (multi-leg GUI)
        std::vector<LiveSnap> v;
        for (const auto& lg : legs_) {
            if (!lg.open) continue;
            LiveSnap s; s.label = lg.label; s.open = true;
            s.armed = (lg.mfe >= lg.arm); s.peak_mfe_pct = lg.mfe;
            s.bars_since_high = (int)std::max<int64_t>(0, cur_bar_ - lg.ext_bar);
            v.push_back(s);
        }
        return v;
    }

    // ── S-2026-07-18ah trigger-PROXIMITY readout (operator ask: "see when a symbol is
    //    moving towards a trigger, not a 24h percentage that means nothing") ──────────
    // Read-only view of the self-detector's CURRENT window move vs its own threshold:
    //   j_pct  = det_close_/ring.front - 1 — the RUNNING close (updates every tick via
    //            feed_selfdetect_), so the desk sees intra-bar drift toward the trigger,
    //            not just the last H1 close. At bar close this exact j is what enters.
    //   win_open      = detector window open (det_in_ / jf_in_).
    //   confirm_dist_bp = window OPEN + book still flat: bp of upward move still needed
    //            to cross the lowest per-tier confirm (the first REAL booked entry).
    //            <=0 means confirm crossed / legs live. 0 when window closed.
    // det_w==0 (parent-driven) cells report valid=false — their trigger belongs to the
    // parent's UNIFORM 4h window family, never conflate (feedback-test-operator-spec).
    struct DetProximity {
        bool   valid = false;          // self-detect cell with a FULL ring (W+1 closes)
        bool   win_open = false;
        double j_pct = 0.0;            // current window move, %
        double thr_pct = 0.0;          // det_thr, %
        double confirm_dist_bp = 0.0;
    };
    DetProximity det_proximity() const {
        DetProximity p;
        p.thr_pct  = cfg_.det_thr * 100.0;
        p.win_open = det_in_ || jf_in_;
        if (cfg_.det_w > 0 && (int)h1c_.size() >= cfg_.det_w + 1
            && h1c_.front() > 0.0 && det_close_ > 0.0) {
            p.valid = true;
            p.j_pct = (det_close_ / h1c_.front() - 1.0) * 100.0;
        }
        if (p.win_open && det_entry_ > 0.0 && det_close_ > 0.0) {
            bool flat = true;
            for (const auto& lg : legs_) if (lg.open) { flat = false; break; }
            if (flat) {
                const double conf_px = det_entry_ * (1.0 + min_confirm_bp_() / 1e4);
                p.confirm_dist_bp = (conf_px / det_close_ - 1.0) * 1e4;
            }
        }
        return p;
    }

private:
    // ── internal mimic detector (be_floor self-detect; parent never read) ──
    // Aggregates H1 closes from the mark stream; a bar "closes" when the next bar's
    // first mark arrives. On each completed close: run sw.parent(det_w,det_thr) and
    // drive the leg book with the detector's own (in_event, entry_px) window.
    void feed_selfdetect_(double cur_px, int64_t ts_ms) {
        if (cur_px <= 0.0) return;
        const int64_t bar = ts_ms / (cfg_.tf_secs * 1000);
        if (det_bar_ < 0) { det_bar_ = bar; det_close_ = cur_px; return; }
        if (bar < det_bar_) return;                             // ignore stale/backward feeds (2nd driver, rehydrate replay)
        if (bar == det_bar_) { det_close_ = cur_px; return; }   // same bar -> update running close
        process_close_(det_close_, det_bar_);                   // prior bar finalized
        det_bar_ = bar; det_close_ = cur_px;
    }
    void process_close_(double close, int64_t closed_bar) {
        h1c_.push_back(close);
        if ((int)h1c_.size() > cfg_.det_w + 1) h1c_.erase(h1c_.begin());
        if (cfg_.jump_floor) { jf_on_close_(close, closed_bar); return; }   // JUMP-FLOOR: own state machine
        if ((int)h1c_.size() >= cfg_.det_w + 1) {               // have close[i] and close[i-W]
            const double past = h1c_.front();
            const double j = close / past - 1.0;
            // RANK-OUT / RETIRED books take NO NEW windows (enter suppressed);
            // an in-flight window still exits normally so open legs flush.
            if (!det_in_ && j >=  cfg_.det_thr) {
                if (arming_allowed_()) { det_in_ = true; det_entry_ = close; }           // enter (entry ~ next open)
            }
            else if (det_in_ && j <= -cfg_.det_thr) { det_in_ = false; }                 // exit -> book flushes
        }
        const int64_t ts = closed_bar * cfg_.tf_secs * 1000;
        if (cfg_.be_floor) observe_be_(det_in_, det_entry_, close, ts, closed_bar);      // BE-floor book
        else               observe_ladder_(det_in_, det_entry_, close, ts, closed_bar);  // LADDER book (S-2026-07-07w)
    }

    // ── JUMP-FLOOR mode (S-2026-07-14, faithful to mimic2pct_be_bt.cpp percoin cells) ──
    // Single position per detected event. det_in_/det_entry_ mirror jf_in_/jf_E_ so
    // det_state_json()/restore_det_state() persistence works unchanged. legs_ carries
    // ONE display-only Leg ("J1") while in position so snapshot()/leg_snapshots()
    // render on the desk with zero GUI changes (arm=0.2% => armed chip == floored).
    void jf_on_close_(double close, int64_t closed_bar) {
        if ((int)h1c_.size() < cfg_.det_w + 1) return;          // ring warming (W+1 closes)
        const double j  = close / h1c_.front() - 1.0;
        const double rt = cfg_.round_trip_bp * 1e-4;
        cur_bar_ = closed_bar;
        if (!jf_in_ && !jf_pending_open_) {
            if (!jf_armed_) { if (j < cfg_.det_thr) jf_armed_ = true; return; }   // fresh-jump re-arm (BT `armed`)
            if (j >= cfg_.det_thr && arming_allowed_()) {
                jf_pending_open_ = true; jf_armed_ = false;     // fill at next tick == BT next open
            }
            return;
        }
        if (!jf_in_) return;                                    // pending open, nothing to manage yet
        const double fav = close / jf_E_ - 1.0;
        if (fav > jf_mfe_) { jf_mfe_ = fav; jf_ext_bar_ = closed_bar; }
        if (!jf_floored_ && fav >= rt) jf_floored_ = true;      // close covered cost -> floor arms
        if (jf_floored_) {
            double ns = jf_E_ * (1.0 + rt);                     // BE floor incl. cost
            if (cfg_.jf_giveback < 1.0)
                ns = std::max(ns, jf_E_ * (1.0 + jf_mfe_ * (1.0 - cfg_.jf_giveback)));
            if (ns > jf_stop_) jf_stop_ = ns;                   // stop only ratchets up
        }
        if (!legs_.empty()) { legs_[0].mfe = jf_mfe_ * 100.0; legs_[0].ext_bar = jf_ext_bar_; }
        if (j <= -cfg_.det_thr) jf_pending_exit_ = true;        // reversal -> exit at next tick (BT next open)
        det_in_ = jf_in_; det_entry_ = jf_E_;                   // persistence mirror
    }
    void jf_on_tick_(double px, int64_t ts_ms) {
        if (px <= 0.0) return;
        if (jf_pending_open_) {                                 // first tick after detect close == BT next open
            jf_pending_open_ = false;
            jf_in_ = true; jf_E_ = px; jf_mfe_ = 0.0; jf_floored_ = false;
            jf_stop_ = (cfg_.jf_prebe_stop_bp > 0.0) ? px * (1.0 - cfg_.jf_prebe_stop_bp / 1e4) : -1.0;
            jf_open_ts_ = ts_ms; jf_open_bar_ = ts_ms / (cfg_.tf_secs * 1000);
            jf_ext_bar_ = jf_open_bar_;
            det_in_ = true; det_entry_ = jf_E_;                 // persistence mirror
            legs_.clear();                                      // ONE display leg for the desk panel
            Leg l; l.label = "J1"; l.epx = jf_E_; l.le = jf_E_; l.arm = cfg_.round_trip_bp / 100.0;
            l.open = true; l.open_ts = ts_ms; l.open_bar = jf_open_bar_; l.ext_bar = jf_open_bar_;
            legs_.push_back(l);
            announce_open_(legs_.back(), ts_ms, jf_E_);     // live mirror (jump-floor immediate entry)
            std::printf("[CLIP-JF] %s OPEN px=%.6f stop=%s (det j>=+%.2f%%) shadow=%d\n",
                cfg_.tag.c_str(), jf_E_,
                jf_stop_ > 0 ? "preBE" : "none", cfg_.det_thr * 100.0, shadow_mode ? 1 : 0);
            std::fflush(stdout);
            return;
        }
        if (!jf_in_) return;
        if (jf_restored_) {                                     // first tick after a RESTART restore
            jf_restored_ = false;
            // A restored leg whose stop is already breached on the FIRST observed tick means the
            // market moved during downtime; the leg would have exited at the floor in continuous live
            // trading. Book at the FLOOR (jf_stop_), never the post-outage px -> honors the >=BE floor
            // invariant. NOT the retired be_floor tautology: this fires ONLY on the restart-gap first
            // tick; every live intrabar stop below still books at the honest px.
            if (jf_stop_ > 0.0 && px <= jf_stop_) {
                jf_book_(jf_stop_, ts_ms, jf_floored_ ? "RESTORE_FLOOR" : "RESTORE_PREBE");
                return;
            }
        }
        if (jf_pending_exit_) { jf_book_(px, ts_ms, "REVERSAL_EXIT"); return; }   // booked at next tick px
        if (jf_stop_ > 0.0 && px <= jf_stop_)                   // intrabar stop: pre-BE / floor / trail
            jf_book_(px, ts_ms, jf_floored_ ? "FLOOR_TRAIL_STOP" : "PREBE_STOP");
    }
    void jf_book_(double px, int64_t ts_ms, const char* reason) {
        const double gross = (px / jf_E_ - 1.0) * 1e4;
        const double net   = gross - cfg_.round_trip_bp;        // ONE honest column: real == model, no clamp
        ClipRecord r;
        r.tag = cfg_.tag + "-J1"; r.symbol = cfg_.symbol; r.reason = reason;
        r.entry_ts_ms = jf_open_ts_; r.exit_ts_ms = ts_ms;
        r.entry_px = jf_E_; r.exit_px = px;
        r.gross_bp = gross; r.net_bp = net; r.mfe_pct = jf_mfe_ * 100.0;
        r.gross_bp_real = gross; r.net_bp_real = net;
        r.size_mult = cfg_.size_mult;
        r.bars_held = (int)(ts_ms / (cfg_.tf_secs * 1000) - jf_open_bar_);
        r.clip_num = ++clip_num_;
        r.shadow = shadow_mode;
        r.signal_only = cfg_.signal_only;
        // S-17s SIGNAL-ONLY: no bank — the cell trades nothing, it only drives its mimic.
        if (!cfg_.signal_only) {
            banked_bp_ += net; banked_bp_real_ += net; banked_bp_real_w_ += net * cfg_.size_mult;
        }
        const double bE = jf_E_; const double bmfe = jf_mfe_;   // for the log line below
        // S-2026-07-17 ORDER FIX (ETH-PJ7W24 zombie root-root cause): position state must be
        // CLEARED BEFORE on_clip_ fires. Fig-2's immediate det-state persist runs INSIDE the
        // on_clip_ handler (main.cpp persist_companion_clip -> save_companion_det_state); with
        // the old order (clear AFTER on_clip_) every "immediate persist" serialized det_in:1 —
        // the pre-close state — so the file NEVER recorded the close and the leg resurrected on
        // every restart despite Fig-2 being deployed. Clear first; the ClipRecord r already
        // carries everything the handler needs.
        jf_in_ = false; jf_pending_exit_ = false; jf_pending_open_ = false;
        jf_E_ = 0.0; jf_mfe_ = 0.0; jf_floored_ = false; jf_stop_ = -1.0;
        det_in_ = false; det_entry_ = 0.0;                      // persistence mirror
        legs_.clear();                                          // drop the display leg
        // jf_armed_ stays false: the next entry needs a close with j < det_thr first (fresh jump)
        if (on_clip_) on_clip_(r);                       // signal_only: handler persists det-state ONLY
        if (!cfg_.signal_only) check_retire_();          // a bookless cell never retires (signal must persist)
        std::printf("[CLIP][%s] %s net=%+.1fbp gross=%+.1fbp mfe=%.2f%% bars=%d px %.6f->%.6f shadow=%d %s\n",
            r.tag.c_str(), reason, net, gross, bmfe * 100.0, r.bars_held, bE, px, shadow_mode ? 1 : 0,
            cfg_.signal_only ? "JUMPFLOOR-SIGNAL-ONLY(not booked)" : "JUMPFLOOR");
        std::fflush(stdout);
    }

    // ── BE-FLOOR mode (faithful port of be_bptrail.py leg_book) ──────────────
    // net_bp >= 0 on EVERY clip by construction; no ladder; reclip from exit px.
    void observe_be_(bool parent_in_pos, double parent_entry_px, double cur_px, int64_t ts_ms, int64_t bar) {
        cur_bar_ = bar;
        // Parent flat / bad mark -> flush every open leg (floored >=0), reset.
        if (!parent_in_pos || parent_entry_px <= 0.0 || cur_px <= 0.0) {
            const double px = (cur_px > 0.0) ? cur_px : last_be_px_;
            for (auto& lg : legs_) flush_be_(lg, px, ts_ms, bar);
            reset_session_();
            return;
        }
        // New parent trade -> flush stragglers, reset, seed 2 base legs (ref = parent entry).
        if (entry_ref_ != parent_entry_px) {
            for (auto& lg : legs_) flush_be_(lg, cur_px, ts_ms, bar);
            reset_session_();
            entry_ref_ = parent_entry_px;
            legs_.push_back(make_be_leg_("T1", parent_entry_px, cfg_.tight));
            legs_.push_back(make_be_leg_("T2", parent_entry_px, cfg_.wide));
        }
        last_be_px_ = cur_px;
        for (auto& lg : legs_) step_be_(lg, cur_px, ts_ms, bar);   // NO ladder spawn in be_floor
    }

    Leg make_be_leg_(std::string label, double ref_px, const Tier& t) {
        Leg l; l.label = std::move(label);
        l.ref_px = ref_px; l.trail_bp = t.trail_bp; l.arm = t.arm;
        l.open = false; l.clipped = false; l.le = 0.0; l.hwm = 0.0;
        return l;
    }

    // one H1-close step of a BE-floor leg. Books a clip (net>=0) when the trailing
    // stop (floored at entry) is hit; reclips from the exit price.
    void step_be_(Leg& lg, double cur, int64_t ts, int64_t bar) {
        if (!lg.open) {                                     // FLAT: wait for +be_bp from ref -> open here
            if ((cur / lg.ref_px - 1.0) * 1e4 < cfg_.be_bp) return;
            lg.open = true; lg.le = cur; lg.hwm = cur;
            lg.open_ts = ts; lg.open_bar = bar; lg.ext_bar = bar; lg.mfe = 0.0;
            announce_open_(lg, ts, cur);                    // live mirror
            return;                                         // opened this bar; stop==le, cur==le -> no exit yet
        }
        if (cur > lg.hwm) { lg.hwm = cur; lg.ext_bar = bar; }
        lg.mfe = (lg.hwm / lg.le - 1.0) * 100.0;            // peak % from entry (for the GUI snapshot)
        const double stop = (lg.trail_bp > 0.0)
            ? std::max(lg.le, lg.hwm * (1.0 - lg.trail_bp / 1e4))
            : lg.le;                                        // trail_bp==0 -> pure BE floor (ride to parent exit)
        if (cur <= stop) {
            const double gross = std::max(0.0, (stop / lg.le - 1.0) * 1e4);   // >=0 ALWAYS (MODEL column, reference only)
            const double gross_real = (cur / lg.le - 1.0) * 1e4;              // REAL: mechanism is H1-close-driven -> honest fill = the close that tripped it (worse-of)
            emit_be_clip_(lg, stop, ts, bar, gross, gross_real, "BE_TRAIL_CLIP");
            lg.ref_px = stop; lg.open = false; lg.clipped = false;            // reclip from exit px
            lg.le = 0.0; lg.hwm = 0.0;
        }
    }

    void flush_be_(Leg& lg, double px, int64_t ts, int64_t bar) {            // parent exit: floored, never underwater
        if (!lg.open || lg.le <= 0.0 || px <= 0.0) return;
        const double gross = std::max(0.0, (px / lg.le - 1.0) * 1e4);        // MODEL (clamped, reference only)
        const double gross_real = (px / lg.le - 1.0) * 1e4;                  // REAL: unclamped MTM at the flush px
        emit_be_clip_(lg, px, ts, bar, gross, gross_real, "ENGINE_EXIT");
        lg.open = false;
    }

    void emit_be_clip_(Leg& lg, double exit_px, int64_t ts, int64_t bar,
                       double gross, double gross_real, const char* reason) {
        const double net = gross;   // MODEL column: "cost already paid by +be_bp" fallacy — kept as reference, NEVER fold
        const double net_real = gross_real - cfg_.round_trip_bp;   // REAL: worse-of fill − RT cost (the honest book)
        ClipRecord r;
        r.tag = cfg_.tag + "-" + lg.label; r.symbol = cfg_.symbol; r.reason = reason;
        r.entry_ts_ms = lg.open_ts; r.exit_ts_ms = ts;
        r.entry_px = lg.le; r.exit_px = exit_px;
        r.gross_bp = gross; r.net_bp = net; r.mfe_pct = lg.mfe;
        r.gross_bp_real = gross_real; r.net_bp_real = net_real;
        r.size_mult = cfg_.size_mult;
        r.bars_held = (int)(bar - lg.open_bar);
        r.clip_num = ++clip_num_;
        r.shadow = shadow_mode;
        banked_bp_ += net;
        banked_bp_real_ += net_real;
        banked_bp_real_w_ += net_real * cfg_.size_mult;
        if (on_clip_) on_clip_(r);
        check_retire_();
        std::printf("[CLIP][%s] %s real=%+.1fbp (model=%+.1fbp) gross_real=%+.1fbp mfe=%.2f%% bars=%d px %.6f->%.6f shadow=%d BEFLOOR\n",
            r.tag.c_str(), reason, net_real, net, gross_real, lg.mfe, r.bars_held, lg.le, exit_px, shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    // ── S-2026-07-08 weighting / auto-retirement helpers ─────────────────────
    bool arming_allowed_() const { return !cfg_.rank_out && !retired_; }
    void check_retire_() {
        if (retired_ || cfg_.retire_bp >= 0.0) return;               // off / already latched
        if (banked_bp_real_ > cfg_.retire_bp) return;                // raw REAL bank above threshold
        if (cfg_.retire_override) {
            std::printf("[CLIP-RETIRE-OVERRIDE] %s bank_real=%.1fbp <= retire_bp=%.1fbp but tag is in "
                        "companion_unretire.flags — retirement DISABLED this run (operator act)\n",
                        cfg_.tag.c_str(), banked_bp_real_, cfg_.retire_bp);
            std::fflush(stdout);
            return;
        }
        retired_ = true;                                             // one-shot latch
        std::printf("[CLIP-RETIRE] *** %s AUTO-RETIRED: banked REAL net %.1fbp <= threshold %.1fbp "
                    "(-2x worst validated BT drawdown). NO NEW windows will arm; open legs manage "
                    "normally. Un-retire = operator act: add tag to data/companion_unretire.flags "
                    "(or archive the clip ledger) and restart. ***\n",
                    cfg_.tag.c_str(), banked_bp_real_, cfg_.retire_bp);
        std::fflush(stdout);
    }

    void reset_session_() { legs_.clear(); entry_ref_ = 0.0; last_stagger_bar_ = 0; }

    // STAGGER: release (make eligible) the next base tier leg when its gate is met.
    // BE_CASCADE (mode 1): hold until every already-released leg is OPEN and BE'd
    //   (mfe >= stagger_be_bp) -> release exactly ONE more (=> at most 1 un-BE'd leg).
    // TIME (mode 2): release one leg every stagger_k bars since the last release.
    // Ineligible (not-yet-released) legs never open (step_leg_ early-returns on them).
    void advance_stagger_(int64_t bar) {
        if (cfg_.stagger_mode == 0) return;
        Leg* next = nullptr;                                   // lowest-tier_idx not-yet-eligible leg
        for (auto& lg : legs_) if (!lg.eligible && (!next || lg.tier_idx < next->tier_idx)) next = &lg;
        if (!next) return;
        if (cfg_.stagger_mode == 1) {                          // BE_CASCADE
            for (auto& lg : legs_) {
                if (!lg.eligible) continue;
                if (!(lg.open && lg.mfe >= cfg_.stagger_be_bp / 100.0)) return;  // an eligible leg not yet BE'd -> hold
            }
            next->eligible = true;                             // all eligible legs open+BE'd -> release exactly one
        } else if (cfg_.stagger_mode == 2) {                   // TIME
            if (bar - last_stagger_bar_ >= cfg_.stagger_k) { next->eligible = true; last_stagger_bar_ = bar; }
        }
    }

    void init_base_legs_(double epx, int64_t ts, int64_t bar) {
        legs_.push_back(make_leg_("T1", epx, cfg_.tight, ts, bar, false));
        // MIMIC-FLOOR = ONE managed position per event (operator: trail from peak, exit on
        // giveback, re-enter). Extra arm tiers are meaningless under a single-g floor trail
        // (arm level is unused once the floor governs exits) — they would just N-multiply an
        // identical clip. So a plain mimic_floor cell runs a single T1 leg (== jump_floor's J1).
        // EXCEPTION (S-2026-07-16 mimic_stagger): the extra tiers carry ESCALATING per-tier
        // `confirm` (BE / +1% / +2% / ...), so each opens at a DIFFERENT price and floors at its
        // OWN fill — distinct additive positions, not the redundant same-entry N-multiply. Fall
        // through to build them; each still routes through the per-leg BE-floor trail.
        if (cfg_.mimic_floor && !cfg_.mimic_stagger) return;
        legs_.push_back(make_leg_("T2", epx, cfg_.wide,  ts, bar, false));
        int i = 0;   // stacked base arms (item 5): one more base leg per extra tier
        for (const auto& t : cfg_.extra_base)
            legs_.push_back(make_leg_("S" + std::to_string(++i), epx, t, ts, bar, false));
        // STAGGER: number the base tiers in open order; hold all but the first FLAT
        // (advance_stagger_ releases them per BE-cascade / time rule). mode 0 = every
        // leg stays eligible (unchanged for all non-staggered cells).
        if (cfg_.stagger_mode != 0) {
            for (size_t k = 0; k < legs_.size(); ++k) {
                legs_[k].tier_idx = (int)k;
                legs_[k].eligible = (k == 0);
            }
            last_stagger_bar_ = bar;   // TIME: releases measured from the window-entry bar
        }
    }

    Leg make_leg_(std::string label, double epx, const Tier& t, int64_t /*ts*/, int64_t /*bar*/, bool /*seed*/) {
        Leg l; l.label = std::move(label);
        l.epx = epx; l.le = epx; l.arm = t.arm; l.stall = t.stall; l.gb = t.gb;
        l.rc = cfg_.reclip_pct; l.cg = cfg_.cost_gate_bp;
        l.confirm = (t.confirm > 0.0 ? t.confirm : cfg_.confirm_bp);   // per-tier BE-entry stagger (S-2026-07-16)
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
        if (!lg.eligible) return false;                 // STAGGER: leg not yet released -> stays FLAT
        if (lg.clipped) {
            if (lg.rc > 0.0 && lg.pk > 0.0 && fav > lg.pk * (1.0 + lg.rc)) {
                lg.clipped = false; lg.le = cur;      // RECLIP = re-enter at current price
                if (cfg_.mimic_floor) { lg.hwm = cur; lg.floored = false; lg.stop_px = -1.0; }  // fresh floor on the new fill
                announce_open_(lg, bar * cfg_.tf_secs * 1000, cur);   // reclip = a NEW live fill for the mirror
            } else return false;
        }
        if (!lg.open) {
            if (lg.confirm > 0.0 && fav * 100.0 < lg.confirm) return false;   // OPTION-B: not yet confirmed -> stay flat, book nothing
            lg.open = true; lg.open_ts = bar * cfg_.tf_secs * 1000; lg.open_bar = bar;
            if ((lg.confirm > 0.0 && !cfg_.confirm_anchor_epx) || lg.seeded_flat) lg.le = cur;  // le = confirm price / first live mark (rehydrate-FLAT: never backdated). confirm_anchor_epx keeps le=epx so the leg is floored-on-open at window-entry BE (no pre-arm loss).
            lg.mfe = fav; lg.ext_bar = bar;
            if (cfg_.mimic_floor) { lg.hwm = lg.le; lg.floored = false; lg.stop_px = -1.0; }     // floor gauged from the fill (le)
            announce_open_(lg, lg.open_ts, cur);   // live mirror buys at the CURRENT mark (cur), not the anchored le
        }
        if (fav > lg.mfe + 1e-9) { lg.mfe = fav; lg.ext_bar = bar; }
        // ── MIMIC-FLOOR: BE-floored HWM-trail replaces the floorless gb/stall exits ──
        // Ratchet the floor/stop from this close; the actual booking happens intrabar in
        // intrabar_mimic_floor_ (per tick / bar-low) so a fast reversal can't blow the
        // giveback between H1 closes (INJ T1 fix). A close that already sits at/below the
        // ratcheted stop is booked here too (covers the det_w==0 per-tick observe path).
        if (cfg_.mimic_floor) {
            mimic_ratchet_(lg, cur);
            if (lg.floored && lg.stop_px > 0.0 && cur <= lg.stop_px) {
                book_mimic_stop_(lg, cur, cur_ts_(bar), bar);   // book at the ACTUAL trip fill (worse-of on gap)
            }
            return false;   // never routes through the gb/stall return path
        }
        const bool armed = lg.mfe >= lg.arm;
        const int  stall = (int)(bar - lg.ext_bar);
        if (armed && lg.stall > 0 && stall >= lg.stall)                 return clip_leg_(lg, cur, gross_out, reason_out, "STALL_CLIP");
        if (armed && lg.gb > 0.0 && fav <= lg.mfe * (1.0 - lg.gb))      return clip_leg_(lg, cur, gross_out, reason_out, "REVERSAL_CLIP");
        return false;
    }

    // MIMIC-FLOOR helpers — the honest jump_floor floor+trail math, per ladder leg.
    // hwm/floor gauged from le (the fill). Floor arms once a mark covers the leg's own RT
    // cost; the trail gives back fraction g of the run above le but never drops below BE.
    void mimic_ratchet_(Leg& lg, double cur) {
        if (!lg.open || lg.le <= 0.0) return;
        if (cur > lg.hwm) lg.hwm = cur;
        const double rt = cfg_.round_trip_bp * 1e-4;
        if (!lg.floored && lg.hwm >= lg.le * (1.0 + rt)) lg.floored = true;   // covered own cost -> floor arms
        if (!lg.floored) { lg.stop_px = -1.0; return; }                       // pre-arm: no floor (loss_cut bounds tail)
        double ns = lg.le * (1.0 + rt);                                       // BE floor incl. cost
        if (cfg_.mimic_giveback < 1.0)
            ns = std::max(ns, lg.le * (1.0 + (lg.hwm / lg.le - 1.0) * (1.0 - cfg_.mimic_giveback)));
        if (ns > lg.stop_px) lg.stop_px = ns;                                 // stop only ratchets up
    }
    // Book the floored trail exit at the ACTUAL fill that pierced the resting stop.
    void book_mimic_stop_(Leg& lg, double fill_px, int64_t ts, int64_t bar) {
        // S-2026-07-17f FOUNDATION-HONESTY (adversarial-verify finding; feedback-content-parity +
        // feedback-no-prebe-loss-ever): book the ACTUAL fill that pierced the resting stop
        // (worse-of on a gap: bar-low in BT / the trip tick live), NOT lg.stop_px. Booking at the
        // stop LEVEL clamped every gap-through loss to +0 and made the shadow-ledger nNeg=0 a
        // MODEL property, not execution truth — the whole 17c "floored-on-open => nNeg=0 by
        // construction" claim held only in the model (PF=999/nNeg=0 was the mechanically-
        // impossible tell). The DESIGN is still floored-on-open (the resting stop sits at >=BE);
        // this books a real gap-through's sub-BE tail instead of hiding it. fill<=stop_px at the
        // call site; clamp UP to stop_px defensively (a stop can't fill BETTER than its level).
        const double exit_px = (fill_px > 0.0 && fill_px < lg.stop_px) ? fill_px : lg.stop_px;
        const double gross = (exit_px / lg.le - 1.0) * 1e4;
        // fp-noise clamp ONLY: when fill == stop == BE exactly (no gap), IEEE-754 rounding of
        // (le*(1+rt)/le - 1)*1e4 yields ~-1e-7 bp — clamp that sub-bp noise to 0. A REAL gap-
        // through (fill < stop) yields net << -1e-6 and is NOT clamped: it books its true sub-BE
        // tail (the S-17f honesty fix — no longer masked to +0; nNeg>0 now means a real gap tail).
        double net = gross - cfg_.round_trip_bp;
        if (net < 0.0 && net > -1e-6) net = 0.0;
        emit_clip_(lg, exit_px, ts, bar, gross, net, "FLOOR_TRAIL_CLIP");
        if (cfg_.anchored_reclip && cfg_.confirm_anchor_epx && cfg_.reclip_pct > 0.0) {
            // ── ANCHORED RECLIP (S-2026-07-17, feedback-befloor-on-open-foundation pillar 4) ──
            // Re-arm through the confirm+anchor path instead of the raw le=cur reclip (which reopened
            // a pre-arm window and leaked real pre-BE losses). Reset the leg FLAT with a NEW anchor
            // epx = the exit px; step_leg_'s confirm gate re-opens it ONLY once fav(from the new epx)
            // >= confirm, and confirm_anchor_epx keeps le=epx => hwm>=le*(1+RT) on open => floored
            // ON OPEN => net_bp_real >= 0 by construction. epx=exit means the next open needs a fresh
            // >=confirm continuation from where we exited (be_floor reclip parity: ref becomes exit px).
            lg.epx = exit_px; lg.le = exit_px;
            lg.open = false; lg.clipped = false;
            lg.mfe = 0.0; lg.pk = 0.0; lg.ext_bar = bar; lg.open_bar = bar;
            lg.floored = false; lg.stop_px = -1.0; lg.hwm = 0.0;
        } else {
            lg.pk = lg.mfe; lg.clipped = true;             // reclip-eligible from the exit peak (raw le=cur path)
            lg.floored = false; lg.stop_px = -1.0; lg.hwm = 0.0;
        }
    }
    int64_t cur_ts_(int64_t bar) const { return bar * cfg_.tf_secs * 1000; }

    // INTRABAR BE-floored trail stop — runs per tick (live) / on the fed bar-low (backtest),
    // so the giveback fires the instant price crosses the resting stop, not at the next H1
    // close. Books at the stop (>= BE). The ratchet (hwm/floor) is driven by CLOSES in
    // step_leg_/mimic_ratchet_ (jump_floor parity: mfe from closes, stop checked intrabar).
    void intrabar_mimic_floor_(double cur, int64_t ts, int64_t bar) {
        if (cur <= 0.0) return;
        const double lc = cfg_.loss_cut_bp;
        for (auto& lg : legs_) {
            if (!lg.open || lg.clipped || !lg.eligible || lg.le <= 0.0) continue;
            if (lg.floored) {                                    // POST-arm: BE-floored trail stop
                if (lg.stop_px <= 0.0 || cur > lg.stop_px) continue;
                book_mimic_stop_(lg, cur, ts, bar);              // book at the ACTUAL trip fill (worse-of on gap)
            } else if (lc > 0.0) {                               // PRE-arm: hard cut anchored at the FILL (le), NOT epx
                const double cut_px = lg.le * (1.0 - lc / 1e4);  // bounds every pre-arm loss to ~lc+RT from the fill
                if (cur > cut_px) continue;                      // still above the pre-arm stop
                const double gross = (cut_px / lg.le - 1.0) * 1e4;
                emit_clip_(lg, cut_px, ts, bar, gross, gross - cfg_.round_trip_bp, "PREBE_CUT");
                lg.pk = lg.mfe; lg.clipped = true; lg.floored = false; lg.stop_px = -1.0; lg.hwm = 0.0;
            }
        }
    }

    // faithful python Leg._clip — HARD COST-COVER gate suppresses sub-cost clips.
    bool clip_leg_(Leg& lg, double cur, double& gross_out, const char*& reason_out, const char* reason) {
        const double gross = (cur / lg.le - 1.0) * 1e4;
        if (lg.cg > 0.0 && gross < lg.cg) return false;   // cost not covered -> keep holding
        lg.pk = lg.mfe; lg.clipped = true;
        gross_out = gross; reason_out = reason;
        return true;
    }

    // HARD REVERSAL CUT (operator 2026-07-13b, after UNI-MIMH -146bp / -1402 tail): runs on
    // EVERY tick (live) / fed price (backtest LOW), NOT just bar close — so a leg that reverses
    // below entry by loss_cut_bp is cut INTRA-BAR at the stop price, before the parent-reversal
    // flush can book it at a deep bar-close loss. Long-only spot => once below entry the up-move
    // is over; cutting here bounds EVERY loss and is edge-neutral (a leg still above entry never
    // trips it — winners exit via giveback). Cuts ALL open legs (parent + every mimic) together.
    void intrabar_reversal_cut_(double cur, int64_t ts, int64_t bar) {
        if (cur <= 0.0) return;
        for (auto& lg : legs_) {
            if (!lg.open || lg.clipped || !lg.eligible || lg.le <= 0.0 || lg.epx <= 0.0) continue;
            if (cfg_.mimic_floor) continue;                                    // mimic_floor legs are fully managed by intrabar_mimic_floor_ (le-anchored pre-arm cut + BE floor trail)
            const double fav = (cur - lg.epx) / lg.epx * 100.0;              // % from FIXED entry
            if (fav * 100.0 > -cfg_.loss_cut_bp) continue;                    // still above the stop
            const double stop_px = lg.epx * (1.0 - cfg_.loss_cut_bp / 1e4);  // exit ~at the stop
            const double gross   = (stop_px / lg.le - 1.0) * 1e4;
            emit_clip_(lg, stop_px, ts, bar, gross, gross - cfg_.round_trip_bp, "REVERSAL_CUT");
            lg.open = false; lg.clipped = true; lg.pk = lg.mfe;
        }
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
        r.gross_bp_real = gross; r.net_bp_real = net;   // ladder mode fills MTM at cur with cost debited -> model == real
        r.size_mult = cfg_.size_mult;
        r.bars_held = (int)(bar - lg.open_bar);
        r.clip_num = ++clip_num_;
        r.shadow = shadow_mode;
        banked_bp_ += net;
        banked_bp_real_ += net;
        banked_bp_real_w_ += net * cfg_.size_mult;
        if (on_clip_) on_clip_(r);
        check_retire_();
        std::printf("[CLIP][%s] %s net=%+.1fbp gross=%+.1fbp mfe=%.2f%% bars=%d px %.6f->%.6f shadow=%d\n",
            r.tag.c_str(), reason, net, gross, lg.mfe, r.bars_held, lg.le, exit_px, shadow_mode ? 1 : 0);
        std::fflush(stdout);
    }

    // live-mirror announce (S-2026-07-18): live FLAT->OPEN transitions only.
    void announce_open_(const Leg& lg, int64_t ts_ms, double px) {
        if (!on_leg_open_ || cfg_.signal_only) return;
        on_leg_open_({cfg_.tag + "-" + lg.label, cfg_.symbol, lg.label, ts_ms, px,
                      /*floor_anchor*/ lg.le > 0.0 ? lg.le : lg.epx});
    }

public:
    // ── S-2026-07-18s KILL-ALL coverage (LiveMirrorIncident20260718 fix 4) ────
    // /api/kill covered sleeve engines ONLY; companion legs + det windows had to be
    // cleared by hand across 3 files while the GUI kept re-arming them from the det
    // ring. This flushes every open leg MTM (ENGINE_EXIT, honest fill at px), clears
    // the leg book AND closes the detector window (det + jump-floor state) so a
    // subsequent persist writes the disarmed truth and a restart cannot re-arm.
    // Returns the number of legs flushed. Caller drives persist afterwards.
    int kill_disarm(double px, int64_t now_ms) {
        int flushed = 0;
        const int64_t bar = (cfg_.tf_secs > 0) ? now_ms / (cfg_.tf_secs * 1000) : 0;
        const double  fpx = (px > 0.0) ? px : (det_close_ > 0.0 ? det_close_ : entry_ref_);
        for (auto& lg : legs_) {
            if (lg.open && !lg.clipped) { flush_leg_(lg, fpx, now_ms, bar); ++flushed; }
        }
        reset_session_();
        det_in_ = false; det_entry_ = 0.0;                       // close the window
        jf_in_ = false; jf_E_ = 0.0; jf_mfe_ = 0.0; jf_floored_ = false;
        jf_stop_ = -1.0; jf_pending_open_ = false; jf_pending_exit_ = false;
        jf_armed_ = false;                                       // fresh-jump rule: needs a j<thr close to re-arm
        return flushed;
    }

    // ── S-2026-07-20 /api/rearm — intentional-kill recovery ──────────────────
    // After the operator's 23:36Z KILL-ALL test, the fresh-jump rule left every
    // cell disarmed through a 75-min rally (+126..170bp on 5 pilot coins) that the
    // desk structurally could not enter. Operator: missing breakouts unacceptable.
    // This re-opens the entry gate IMMEDIATELY (jf_armed_=true, no j<thr close
    // required): the very next qualifying H1 jump-close can enter. Safe because
    // the BE-ENTRY foundation bounds an aggressive re-arm — a leg books nothing
    // until fav>=confirm and is floored on open, so re-arming mid-move cannot buy
    // a pre-BE loss (it can only re-join late). Deliberately does NOT touch:
    // in-flight legs (guard), the retired_ latch (un-retire = operator act), or
    // the det ring (kill_disarm never cleared h1c_, so jump math is live).
    // be_floor self-detect cells have no armed gate -> no-op false. Caller
    // persists afterwards so a restart restores the ARMED truth.
    bool kill_rearm() {
        if (!cfg_.jump_floor) return false;
        if (jf_in_ || jf_pending_open_) return false;   // never touch an in-flight leg
        const bool was = jf_armed_;
        jf_armed_ = true;
        return !was;
    }

    // ENTRY-gate truth for the desk/heartbeat (S-2026-07-20). NOT the same thing as
    // the snapshot()'s per-leg "armed" — that is the TRAIL-arm (lg.mfe >= lg.arm,
    // always false on a flat book) and misled a live diagnosis into reading 419/419
    // "disarmed" on a healthy armed desk. entry_armed answers "can this cell open on
    // the next qualifying close": non-jump_floor cells have no standing gate (true);
    // jump_floor cells report the fresh-jump gate. In-flight counts as armed.
    bool entry_armed() const noexcept {
        return !cfg_.jump_floor || jf_armed_ || jf_in_ || jf_pending_open_;
    }

private:

    Config        cfg_;
    ClipCallback  on_clip_;
    OpenCallback  on_leg_open_;
    std::vector<Leg> legs_;
    double  entry_ref_ = 0.0;
    int     clip_num_  = 0;
    double  banked_bp_ = 0.0;        // MODEL column (reference only — see gross_bp_real note)
    double  banked_bp_real_ = 0.0;   // HONEST real-fill column — the number that may fold into PnL
    double  banked_bp_real_w_ = 0.0; // weighted real bank (Σ net_bp_real * size_mult) — S-2026-07-08
    bool    retired_ = false;        // auto-retirement latch (S-2026-07-08); un-retire = operator act
    int64_t cur_bar_   = 0;   // last bar seen (for snapshot bars_since_high)
    int64_t last_stagger_bar_ = 0; // TIME-stagger: bar of the last leg release
    double  last_be_px_ = 0.0; // last mark seen in be_floor mode (flush fallback)
    // internal detector state (be_floor self-detect)
    std::vector<double> h1c_;      // ring of last det_w+1 H1 closes
    int64_t det_bar_   = -1;       // current H1 bar being aggregated
    double  det_close_ = 0.0;      // running close of det_bar_
    bool    det_in_    = false;    // in a detected long event
    double  det_entry_ = 0.0;      // event entry ref (~ next open)
    // JUMP-FLOOR mode state (S-2026-07-14; det_in_/det_entry_ mirror for persistence)
    bool    jf_armed_        = true;    // fresh-jump gate: entry allowed (j<thr seen since last exit)
    bool    jf_pending_open_ = false;   // detect close seen -> fill at next tick (BT next open)
    bool    jf_pending_exit_ = false;   // reversal close seen -> book at next tick
    bool    jf_in_           = false;   // position open
    bool    jf_floored_      = false;   // BE floor armed (a close covered entry*(1+RT))
    double  jf_E_    = 0.0;             // entry fill px
    double  jf_mfe_  = 0.0;             // close-based max favorable excursion (fraction)
    double  jf_stop_ = -1.0;            // resting stop px (preBE / floor / trail); <0 = none
    bool    jf_restored_ = false;       // set on a RESTART restore; consumed on the first tick to
                                        // enforce the >=BE floor invariant against a downtime gap
                                        // (a resurrected leg must not book at the post-outage px)
    int64_t jf_open_ts_ = 0, jf_open_bar_ = 0, jf_ext_bar_ = 0;
};

} // namespace chimera

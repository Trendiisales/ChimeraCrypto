// ═══════════════════════════════════════════════════════════════════════════
// CryptoCampaignManager — virtual-lot campaign book, ONE campaign per symbol
// (13j §2.11 component 3; parents per §2.13 / CAMPAIGN_LEVERS_2026-07-13.md)
// ═══════════════════════════════════════════════════════════════════════════
// SHADOW, STANDALONE, ADDITIVE book (never touches, closes or moves any other
// engine's position — feedback-companion-independent-engine). Faithful live
// port of the `campaign` mode in Crypto/backtest/upjump_earlyarm_bt.cpp:1229
// (the harness the 4 PASS parent cells were validated on, 2023-26 H1,
// RT20 + 30/40bp full re-sims + 1-bar delay + 100-seed random-entry control):
//
//   WINDOW  (per cell): j = close[i]/close[i-W] - 1 on H1 closes. j >= thr
//           opens an up-jump window; entry reference px = next bar's OPEN
//           (first mark of the next H1 bar). j <= -thr ends the window
//           (reversal) — any open parent is flushed at that close.
//   PARENT  (ONE per window, ONE campaign per symbol): enters at the H1 close
//           where fav = (close/epx - 1) >= confirm_bp (+20bp CONFIRMED entry —
//           the permitted class; never immediate, never underwater,
//           feedback-no-immediate-entry-upjump-mimic-only). Geometry scales
//           with the structural stop S:
//             stop hit  : intra-tick px <= stop_px (backtest: bar LOW)
//             fee-BE    : MFE >= 0.9*S -> stop = entry*(1 + (RT+3bp))
//             net-lock  : MFE >= 1.8*S -> stop = entry*(1 + (RT+0.4*S))
//             HWM trail : ptrail>0 and MFE >= 2.0*S -> stop = hwm*(1-ptrail)
//             ptrail==0 = ride-to-reversal (floors stay on; TRX cell)
//           MFE/HWM are H1-close-driven (backtest semantics), stops are
//           per-tick (backtest tested stops on bar LOW — live intra-bar
//           stop-touch is the same event seen sooner).
//   MIMIC   lots: **wired OFF** (mimic_enabled=false, no code path opens one).
//           Verdict CAMPAIGN_LEVERS_2026-07-13.md: mechanically sound (funding
//           equation held, combined worst == parent worst in every cell) but
//           ZERO robust standalone H1 edge — all mimic cells fail 1-bar
//           delayed entry. Revisit at tick granularity only.
//
// ADVERSE-PROTECTION (backtested, per cell): structural stop 135/216/111/411bp
// on bar-low fills + fee-BE floor + net-lock + HWM trail, validated with worst
// clip -155/-236/-131/-431bp @20bp RT (re-run 2026-07-13, matches
// CAMPAIGN_LEVERS table). retire_bp per cell ≈ -3x the worst 2x-cost clip —
// auto-retire on banked REAL net breaching it (engine-loss-protection rule).
//
// ONE-CAMPAIGN-PER-SYMBOL FUSION (13j §2.9/§2.5): a manager owns ONE symbol;
// UNI's W1+W2 cells are two detectors feeding one campaign slot — whichever
// confirms first owns it; the other cell cannot enter until the campaign
// closes (per-cell one-entry-per-window is preserved via pdone). This is the
// operator's rule and strictly reduces overlap exposure vs the per-cell BT.
#pragma once

#include "UpJumpLadderCompanion.hpp"   // reuse ClipRecord/LiveSnap contracts
#include "CryptoCostLedger.hpp"
#include "CryptoOpportunityGate.hpp"

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace chimera {

class CryptoCampaignManager {
public:
    using ClipRecord   = UpJumpLadderCompanion::ClipRecord;
    using ClipCallback = UpJumpLadderCompanion::ClipCallback;
    using LiveSnap     = UpJumpLadderCompanion::LiveSnap;

    struct CellCfg {
        std::string tag;          // ledger tag, e.g. "UNI-CAMP-W1"
        std::string cell;         // GUI cell id, e.g. "CW1-3.5"
        int         W = 1;        // up-jump lookback, H1 bars
        double      thr = 0.035;  // window trigger (fraction)
        double      confirm_bp = 20.0;
        double      pstop_bp   = 50.0;   // structural stop S (bp)
        double      ptrail_bp  = 0.0;    // HWM trail; 0 = ride-to-reversal
        double      reset_bp   = 0.0;    // mimic fresh-continuation reset — RESERVED
                                         // (mimic OFF v1; kept so the validated lever
                                         // travels with the cell for the tick revisit)
        double      size_mult  = 1.0;    // size tier (placeholder notional weighting)
        double      retire_bp  = 0.0;    // <0: auto-retire when banked REAL <= this
        double      max_validated_rt_bp = 40.0;  // highest RT the cell's BT re-sim passed
    };

    struct Config {
        std::string symbol;       // e.g. "uniusdt"
        std::string pfx;          // GUI sym, e.g. "UNI"
        int64_t     tf_secs = 3600;
        bool        mimic_enabled = false;   // HARD OFF v1 (no opening code path)
        std::vector<CellCfg> cells;
    };

    CryptoCampaignManager(Config c, CryptoCostLedger* ledger, CryptoOpportunityGate* gate)
        : cfg_(std::move(c)), ledger_(ledger), gate_(gate) {
        cs_.resize(cfg_.cells.size());
        bk_.resize(cfg_.cells.size());
        for (const auto& cc : cfg_.cells) max_w_ = std::max(max_w_, cc.W);
    }

    void set_on_clip(ClipCallback cb) { on_clip_ = std::move(cb); }
    const Config& config() const { return cfg_; }
    int  cell_count() const { return (int)cfg_.cells.size(); }
    bool campaign_open() const { return camp_cell_ >= 0; }
    // read-only entry px of the OPEN campaign (0 if flat) — backtest parent-replay
    // harness reads the REAL campaign window to drive an independent mimic leg.
    double campaign_entry_px() const { return camp_cell_ >= 0 ? pe_ : 0.0; }

    // durable-counter rehydrate per cell (from the companion clip ledger)
    void rehydrate_cell(int ci, int clips, double net, double net_real, double net_real_w) {
        if (ci < 0 || ci >= (int)bk_.size()) return;
        bk_[ci].clips = clips; bk_[ci].bank_bp = net;
        bk_[ci].bank_bp_real = net_real; bk_[ci].bank_bp_real_w = net_real_w;
        check_retire_(ci);
    }

    // ── live drive: every mark for this symbol ──────────────────────────────
    void on_tick(double px, int64_t ts_ms) {
        if (px <= 0.0) return;
        const int64_t bar = ts_ms / (cfg_.tf_secs * 1000);
        if (bar_ < 0) { bar_ = bar; close_ = px; return; }
        if (bar < bar_) return;                    // stale/backward feed
        if (bar > bar_) {
            process_close_(close_, bar_);          // prior bar finalized
            bar_ = bar;
            // first mark of the new bar == the backtest's "next bar OPEN":
            // any window triggered on the finalized close anchors its epx here.
            for (auto& s : cs_)
                if (s.epx_pending) { s.epx = px; s.epx_pending = false; }
        }
        close_ = px;
        // intra-tick structural stop (backtest: bar LOW). Model fill at the
        // stop px (BT convention); REAL column at the worse-of trip px.
        if (camp_cell_ >= 0 && px <= pstop_px_)
            close_campaign_(std::max(px, 0.0), pstop_px_, ts_ms, bar,
                            pstop_px_ > pe_ ? "CAMP_PROT_STOP" : "CAMP_STOP");
    }

    // ── snapshots for the desk panel (same field contract as the companions) ─
    struct CampSnap {
        std::string sym, tag, cell;
        int    det_w = 0; double det_thr = 0.0;
        bool   in_window = false, open = false, armed = false, retired = false;
        double peak_mfe_pct = 0.0; int bars_since_high = 0;
        int    clips = 0;
        double bank_bp = 0.0, bank_bp_real = 0.0, bank_bp_real_w = 0.0, size_mult = 1.0;
    };
    std::vector<CampSnap> snapshots() const {
        std::vector<CampSnap> v;
        for (size_t i = 0; i < cfg_.cells.size(); ++i) {
            const auto& cc = cfg_.cells[i];
            CampSnap s;
            s.sym = cfg_.pfx; s.tag = cc.tag; s.cell = cc.cell;
            s.det_w = cc.W; s.det_thr = cc.thr;
            s.in_window = cs_[i].win_in;
            s.retired = bk_[i].retired;
            s.clips = bk_[i].clips; s.bank_bp = bk_[i].bank_bp;
            s.bank_bp_real = bk_[i].bank_bp_real; s.bank_bp_real_w = bk_[i].bank_bp_real_w;
            s.size_mult = cc.size_mult;
            if (camp_cell_ == (int)i) {
                s.open = true;
                s.armed = pstop_px_ >= pe_;            // protected at-or-above entry
                s.peak_mfe_pct = (phwm_ / pe_ - 1.0) * 100.0;
                s.bars_since_high = (int)std::max<int64_t>(0, bar_ - phwm_bar_);
            }
            v.push_back(s);
        }
        return v;
    }

    // ── state persistence (restart path; same discipline as det-state) ──────
    std::string state_json() const {
        std::ostringstream js; js << std::fixed << std::setprecision(8);
        js << "{\"sym\":\"" << cfg_.symbol << "\",\"bar\":" << bar_
           << ",\"close\":" << close_ << ",\"ring\":[";
        for (size_t i = 0; i < ring_.size(); ++i) { if (i) js << ","; js << ring_[i]; }
        js << "],\"cells\":[";
        for (size_t i = 0; i < cs_.size(); ++i) {
            if (i) js << ",";
            js << "{\"win\":" << (cs_[i].win_in ? 1 : 0)
               << ",\"epx\":" << cs_[i].epx
               << ",\"pend\":" << (cs_[i].epx_pending ? 1 : 0)
               << ",\"pdone\":" << (cs_[i].pdone ? 1 : 0) << "}";
        }
        js << "],\"camp\":{\"cell\":" << camp_cell_ << ",\"pe\":" << pe_
           << ",\"phwm\":" << phwm_ << ",\"pstop\":" << pstop_px_
           << ",\"ets\":" << entry_ts_ms_ << ",\"ebar\":" << entry_bar_
           << ",\"hbar\":" << phwm_bar_ << "}}";
        return js.str();
    }
    // Restore verbatim. Stale state (>24 H1 bars old) is DISCARDED — an
    // ancient snapshot must not inject a phantom window/campaign (the
    // rehydrate lesson from LATEARM_VERDICT.md, applied here from day one).
    void restore_state(const std::string& line, int64_t now_ms) {
        auto num = [&](const char* key, double dflt) -> double {
            auto p = line.find(key); if (p == std::string::npos) return dflt;
            try { return std::stod(line.substr(p + std::strlen(key))); } catch (...) { return dflt; }
        };
        const int64_t saved_bar = (int64_t)num("\"bar\":", -1);
        const int64_t now_bar   = now_ms / (cfg_.tf_secs * 1000);
        if (saved_bar < 0 || now_bar - saved_bar > 24) {
            std::printf("[CAMP-SEED] %s: saved state stale (%lld bars) — cold start\n",
                        cfg_.pfx.c_str(), (long long)(saved_bar < 0 ? -1 : now_bar - saved_bar));
            return;
        }
        bar_ = saved_bar; close_ = num("\"close\":", 0.0);
        ring_.clear();
        auto rp = line.find("\"ring\":[");
        if (rp != std::string::npos) {
            auto re = line.find("]", rp);
            std::stringstream ss(line.substr(rp + 8, re - rp - 8)); std::string tok;
            while (std::getline(ss, tok, ','))
                if (!tok.empty()) { try { ring_.push_back(std::stod(tok)); } catch (...) {} }
        }
        if ((int)ring_.size() > max_w_ + 1)
            ring_.erase(ring_.begin(), ring_.end() - (max_w_ + 1));
        // per-cell window state: objects are in declaration order in the json
        auto cp = line.find("\"cells\":[");
        if (cp != std::string::npos) {
            size_t pos = cp;
            for (auto& s : cs_) {
                auto op = line.find("{", pos + 1);
                if (op == std::string::npos) break;
                auto oe = line.find("}", op);
                std::string obj = line.substr(op, oe - op + 1);
                auto onum = [&](const char* key, double dflt) -> double {
                    auto p = obj.find(key); if (p == std::string::npos) return dflt;
                    try { return std::stod(obj.substr(p + std::strlen(key))); } catch (...) { return dflt; }
                };
                s.win_in      = onum("\"win\":", 0) > 0.5;
                s.epx         = onum("\"epx\":", 0.0);
                s.epx_pending = onum("\"pend\":", 0) > 0.5;
                s.pdone       = onum("\"pdone\":", 0) > 0.5;
                pos = oe;
            }
        }
        camp_cell_ = (int)num("\"cell\":", -1);
        if (camp_cell_ >= (int)cfg_.cells.size()) camp_cell_ = -1;
        if (camp_cell_ >= 0) {
            pe_ = num("\"pe\":", 0.0); phwm_ = num("\"phwm\":", 0.0);
            pstop_px_ = num("\"pstop\":", 0.0);
            entry_ts_ms_ = (int64_t)num("\"ets\":", 0);
            entry_bar_   = (int64_t)num("\"ebar\":", bar_);
            phwm_bar_    = (int64_t)num("\"hbar\":", bar_);
            if (pe_ <= 0.0 || pstop_px_ <= 0.0) camp_cell_ = -1;   // corrupt -> no campaign
        }
        std::printf("[CAMP-SEED] %s restored: ring=%zu camp_cell=%d%s\n",
                    cfg_.pfx.c_str(), ring_.size(), camp_cell_,
                    camp_cell_ >= 0 ? " (open campaign resumed verbatim)" : "");
    }

private:
    struct CellState {
        bool   win_in = false;       // inside an up-jump window
        double epx = 0.0;            // window entry reference px (next-bar open)
        bool   epx_pending = false;  // trigger fired; epx anchors at next mark
        bool   pdone = false;        // parent already entered this window
    };
    struct Bank {
        int clips = 0;
        double bank_bp = 0.0, bank_bp_real = 0.0, bank_bp_real_w = 0.0;
        bool retired = false;
    };

    // one finalized H1 close — the backtest's bar loop body, in close order
    void process_close_(double close, int64_t closed_bar) {
        ring_.push_back(close);
        if ((int)ring_.size() > max_w_ + 1) ring_.erase(ring_.begin());
        const int64_t close_ts = (closed_bar + 1) * cfg_.tf_secs * 1000;

        // 1) window transitions per cell (j on H1 closes, faithful parent())
        for (size_t i = 0; i < cs_.size(); ++i) {
            auto& s = cs_[i];
            const int W = cfg_.cells[i].W;
            if ((int)ring_.size() < W + 1) continue;
            const double j = close / ring_[ring_.size() - 1 - W] - 1.0;
            if (!s.win_in && j >= cfg_.cells[i].thr) {
                s.win_in = true; s.epx = 0.0; s.epx_pending = true; s.pdone = false;
            } else if (s.win_in && j <= -cfg_.cells[i].thr) {
                // reversal — window over; flush an open campaign owned by this cell
                if (camp_cell_ == (int)i)
                    close_campaign_(close, close, close_ts, closed_bar, "CAMP_WINDOW_EXIT");
                s.win_in = false; s.epx_pending = false; s.pdone = false;
            }
        }

        // 2) parent ladder on the close (HWM/MFE are close-driven, BT-faithful)
        if (camp_cell_ >= 0) {
            const auto& cc = cfg_.cells[camp_cell_];
            const double rt = ledger_->net_rt_bp(cfg_.symbol);
            if (close > phwm_) { phwm_ = close; phwm_bar_ = closed_bar; }
            const double pmfe = (phwm_ / pe_ - 1.0) * 1e4;
            const double S = cc.pstop_bp;
            if (pmfe >= 0.9 * S)
                pstop_px_ = std::max(pstop_px_, pe_ * (1.0 + (rt + 3.0) / 1e4));
            if (pmfe >= 1.8 * S)
                pstop_px_ = std::max(pstop_px_, pe_ * (1.0 + (rt + 0.4 * S) / 1e4));
            if (cc.ptrail_bp > 0 && pmfe >= 2.0 * S)
                pstop_px_ = std::max(pstop_px_, phwm_ * (1.0 - cc.ptrail_bp / 1e4));
        }

        // 3) confirmed entry — only while NO campaign is open on this symbol
        if (camp_cell_ < 0) {
            for (size_t i = 0; i < cs_.size(); ++i) {
                auto& s = cs_[i];
                if (!s.win_in || s.pdone || s.epx_pending || s.epx <= 0.0) continue;
                const double fav = (close / s.epx - 1.0) * 1e4;
                if (fav < cfg_.cells[i].confirm_bp) continue;
                const auto d = gate_->check(ledger_->effective_rt_bp(cfg_.symbol),
                                            cfg_.cells[i].max_validated_rt_bp,
                                            bk_[i].retired, /*rank_out*/ false);
                if (!d.allow) {
                    // one-shot per window: mark done so a blocked window doesn't
                    // re-test every close (the BT's one-entry-per-window shape)
                    s.pdone = true;
                    std::printf("[CAMP-GATE] %s %s entry blocked: %s\n",
                                cfg_.pfx.c_str(), cfg_.cells[i].tag.c_str(), d.reason);
                    continue;
                }
                camp_cell_ = (int)i; pe_ = close; phwm_ = close;
                pstop_px_ = pe_ * (1.0 - cfg_.cells[i].pstop_bp / 1e4);
                entry_ts_ms_ = close_ts; entry_bar_ = closed_bar; phwm_bar_ = closed_bar;
                s.pdone = true;   // ONE parent entry per window
                std::printf("[CAMP-OPEN] %s %s pe=%.6f stop=%.6f (S=%.0fbp, confirmed +%.0fbp)\n",
                            cfg_.pfx.c_str(), cfg_.cells[i].tag.c_str(), pe_, pstop_px_,
                            cfg_.cells[i].pstop_bp, fav);
                break;            // one campaign per symbol
            }
        }
        // MIMIC lots: cfg_.mimic_enabled is false in v1 and there is no code
        // path that opens one — see header verdict (revisit at tick scale).
    }

    void close_campaign_(double trip_px, double model_px, int64_t ts_ms,
                         int64_t bar, const char* reason) {
        const int ci = camp_cell_;
        camp_cell_ = -1;
        if (ci < 0 || pe_ <= 0.0) return;
        const auto& cc = cfg_.cells[ci];
        const double rt = ledger_->net_rt_bp(cfg_.symbol);
        const double gross      = (model_px / pe_ - 1.0) * 1e4;   // BT convention (stop px)
        const double real_px    = (trip_px > 0.0) ? std::min(trip_px, model_px) : model_px;
        const double gross_real = (real_px / pe_ - 1.0) * 1e4;    // worse-of fill (honest column)
        auto& b = bk_[ci];
        b.clips++;
        b.bank_bp        += gross - rt;
        b.bank_bp_real   += gross_real - rt;
        b.bank_bp_real_w += (gross_real - rt) * cc.size_mult;
        ClipRecord r;
        r.tag = cc.tag; r.symbol = cfg_.symbol; r.reason = reason;
        r.entry_ts_ms = entry_ts_ms_; r.exit_ts_ms = ts_ms;
        r.entry_px = pe_; r.exit_px = model_px;
        r.gross_bp = gross; r.net_bp = gross - rt;
        r.gross_bp_real = gross_real; r.net_bp_real = gross_real - rt;
        r.size_mult = cc.size_mult;
        r.mfe_pct = (phwm_ / pe_ - 1.0) * 100.0;
        r.bars_held = (int)std::max<int64_t>(0, bar - entry_bar_);
        r.clip_num = b.clips; r.shadow = true;
        std::printf("[CAMP-CLOSE] %s %s %s net=%.1fbp real=%.1fbp bank_real=%.1fbp\n",
                    cfg_.pfx.c_str(), cc.tag.c_str(), reason,
                    r.net_bp, r.net_bp_real, b.bank_bp_real);
        if (on_clip_) on_clip_(r);
        pe_ = phwm_ = pstop_px_ = 0.0; entry_ts_ms_ = 0;
        check_retire_(ci);
    }

    void check_retire_(int ci) {
        auto& b = bk_[ci];
        const auto& cc = cfg_.cells[ci];
        if (!b.retired && cc.retire_bp < 0.0 && b.bank_bp_real <= cc.retire_bp) {
            b.retired = true;
            std::printf("[CAMP-RETIRE] %s banked REAL %.1fbp <= %.1fbp — cell retired "
                        "(no new campaigns; open campaign manages normally)\n",
                        cc.tag.c_str(), b.bank_bp_real, cc.retire_bp);
        }
    }

    Config cfg_;
    CryptoCostLedger*     ledger_;
    CryptoOpportunityGate* gate_;
    ClipCallback on_clip_;

    // bar aggregation (mark stream -> H1 closes; same pattern as the companions)
    int64_t bar_ = -1; double close_ = 0.0;
    std::vector<double> ring_;         // last max_w_+1 H1 closes
    int max_w_ = 1;

    std::vector<CellState> cs_;        // per-cell window state
    std::vector<Bank>      bk_;        // per-cell durable book

    // the ONE campaign (virtual parent lot)
    int     camp_cell_ = -1;
    double  pe_ = 0.0, phwm_ = 0.0, pstop_px_ = 0.0;
    int64_t entry_ts_ms_ = 0, entry_bar_ = 0, phwm_bar_ = 0;
};

} // namespace chimera

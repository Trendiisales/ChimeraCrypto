// ============================================================================
//  CrossSectionalMomentum2Engine.hpp  —  XSec 2.0  (Phase-5, items 24-27)
//
//  A SEPARATE SHADOW upgrade of CrossSectionalMomentumEngine. It runs ALONGSIDE
//  v1 (does NOT replace it) so the operator sees v1 vs v2.0 forward. Everything
//  here is long-only spot, NO shorts, and — unlike v1's BTC>200d sleeve —
//  **NO 200DMA anywhere** (standing crypto rule). The regime gate is BREADTH
//  (share of the ELIGIBLE universe with positive momentum), never a moving-avg.
//
//  What v2.0 adds over v1 (30d-only rank / fixed 32-sym universe / hard 14d clock):
//   24. COMPOSITE score per eligible coin: vol-adj 7d(.30)+30d(.35)+90d(.20)
//       momentum + accel(.15) − liquidity penalty − correlation-concentration
//       penalty (the last applied greedily at selection).
//   25. ADAPTIVE rebalance: 14d = MAX review, but ALSO rebalance when a held coin
//       drops below top-6, its composite goes negative, a challenger beats the
//       weakest held by a buffer, breadth crosses the regime boundary, or a held
//       position's vol spikes past its allowance. HYSTERESIS: hold while in top-6.
//   26. CORE+CHALLENGER: core 1-3 (~78% of sleeve, inverse-vol) + challenger 4-5
//       (small, ONLY if positive + liquid + cost-separated) + watchlist 6-10 (no
//       position) + CASH when < the required count is positive. Never auto-buys a
//       weak 4th/5th coin.
//   27. DYNAMIC universe: eligible set rebuilt POINT-IN-TIME from listing-age
//       (history length up to i), rolling spot $-volume, and data-quality — no
//       current survivor list used in the past (that would be look-ahead).
//
//  Header-only, dep-free (stdlib only), so the backtest harness + unit tests
//  include it directly. The live path (on_tick -> daily-close -> adaptive
//  rebalance -> callback) reuses the SAME compute_target_weights(); simulate()
//  runs the identical logic over the dense day axis for the BT.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace chimera {

struct XSec2Config {
    // ---- item 24: composite horizons + weights (test starting points) --------
    int    w_short = 7,  w_mid = 30, w_long = 90;
    double wt_short = 0.30, wt_mid = 0.35, wt_long = 0.20, wt_accel = 0.15;
    int    vol_window   = 30;    // realized-vol window for inverse-vol sizing
    double liq_pen_w    = 0.40;  // liquidity penalty (z units subtracted)
    double corr_pen_w   = 0.50;  // correlation-concentration penalty (greedy)
    int    corr_window  = 60;    // window for pairwise return correlation

    // ---- item 26: core+challenger ------------------------------------------
    int    core_k        = 3;    // core 1..3
    int    challenger_k  = 2;    // challenger 4..5 (small)
    double core_frac     = 0.78; // core share of the deployed sleeve
    double challenger_min_z = 0.50; // a challenger must clear this composite (cost-separation)
    int    watch_k       = 5;    // watchlist 6..10 (tracked, no position)
    int    min_positive  = 1;    // need >= this many positive-composite eligible, else CASH
    bool   inverse_vol   = true;

    // ---- item 25: adaptive rebalance + hysteresis ---------------------------
    int    rebalance_days   = 14;   // MAX review clock
    int    min_rebalance_gap = 5;   // event triggers can't fire more often than this (churn guard)
    int    hysteresis_top   = 8;    // hold a name while it stays in the top-8 (wide band = low churn)
    double challenger_buffer = 0.40; // challenger must beat the weakest held by this composite margin
    double vol_breach_mult  = 3.0;  // held vol > entry_vol * this => risk-allowance breach
    // NO-TRADE BAND (backlog execution-quality): skip a rebalance whose two-sided
    // turnover is below this — kills the inverse-vol micro-re-tilt churn that made
    // v2 turn over ~5x more than v1 for no edge. Regime flips always act.
    double no_trade_band    = 0.10;

    // ---- regime gate (BREADTH ONLY — NO 200DMA) ----------------------------
    // Deploy when a SMOOTHED breadth (participation ratio, NOT a price MA) is
    // above the threshold. Optional dead-band via regime_on>0 (off by default —
    // it over-held into the 2022/2025 bears on this data). NO moving-average of
    // price anywhere (standing crypto rule).
    bool   breadth_gate   = true;
    double breadth_thresh = 0.40;   // deploy above this smoothed breadth
    double regime_on      = 0.0;    // >0 enables the ON/OFF dead-band (else single threshold)
    double regime_off     = 0.38;   // dead-band CASH floor (only if regime_on>0)
    int    breadth_smooth = 5;      // days to average breadth (kills single-day flips)

    // ---- item 27: point-in-time eligibility --------------------------------
    int    min_history_days = 120;  // listing-age proxy (history available up to i)
    int    liq_window       = 30;   // rolling window for $-volume
    double min_dollar_vol   = 2.0e6;// avg daily $-vol floor (spot liquidity)
    double dq_min_frac      = 0.80; // fraction of the history window that must be non-nan

    double cost_bps         = 15.0; // per-side turnover cost (SIM only; live uses real fills)
    std::string gate_symbol = "BTC";// unused by the gate (breadth) — kept for parity/telemetry
};

class CrossSectionalMomentum2Engine {
public:
    using RebalanceCallback = std::function<void(int64_t day,
                                                 const std::map<std::string,double>& target_weights,
                                                 bool bull)>;

    explicit CrossSectionalMomentum2Engine(XSec2Config cfg = {}) : cfg_(cfg) {}

    void set_universe(const std::vector<std::string>& syms) {
        for (auto& s : syms) if (!close_.count(s)) { close_[s] = {}; dvol_[s] = {}; }
    }
    void set_rebalance_callback(RebalanceCallback cb) { on_rebalance_ = std::move(cb); }

    // ---- history (warm-seed + live day-rollover). day = unix_ms/86400000. -----
    // dollar_vol = close*base_volume for the day (0 if unknown — liquidity gate
    // then treats it as illiquid, which is the safe/conservative default).
    void seed_daily(const std::string& sym, int64_t day, double close, double dollar_vol) {
        ingest(sym, day, close, dollar_vol);
    }
    // convenience overload (close only — liquidity gate off for that bar)
    void seed_daily_close(const std::string& sym, int64_t day, double close) {
        ingest(sym, day, close, 0.0);
    }

    // ===== POINT-IN-TIME eligibility (item 27) ==============================
    bool eligible(const std::string& sym, size_t i) const {
        auto it = close_.find(sym);
        if (it == close_.end() || i >= it->second.size()) return false;
        const auto& s = it->second;
        if (std::isnan(s[i]) || s[i] <= 0) return false;
        // listing-age / data-quality: enough non-nan closes in the trailing window
        int lo = (int)i - cfg_.min_history_days; if (lo < 0) return false;
        int nn = 0; for (int j = lo; j < (int)i; ++j) if (!std::isnan(s[j]) && s[j] > 0) ++nn;
        if (nn < cfg_.dq_min_frac * cfg_.min_history_days) return false;
        // liquidity: rolling avg daily $-vol over the last liq_window
        double dv = avg_dollar_vol(sym, i, cfg_.liq_window);
        if (cfg_.min_dollar_vol > 0.0 && (std::isnan(dv) || dv < cfg_.min_dollar_vol)) return false;
        return true;
    }
    std::vector<std::string> eligible_set(size_t i) const {
        std::vector<std::string> out;
        for (auto& kv : close_) if (eligible(kv.first, i)) out.push_back(kv.first);
        return out;
    }

    // ===== COMPOSITE score (item 24) ========================================
    // Cross-sectional z-blend of vol-adjusted 7/30/90d momentum + acceleration,
    // minus a liquidity penalty. Correlation-concentration is applied later, at
    // selection (it depends on which names are already picked). Returns the raw
    // per-symbol composite over the eligible set at index i.
    std::map<std::string,double> composite(size_t i, const std::vector<std::string>& elig) const {
        std::map<std::string,double> out;
        if (elig.empty()) return out;
        std::map<std::string,double> f_s, f_m, f_l, f_a, f_liq;
        for (auto& s : elig) {
            const auto& v = close_.at(s);
            double vs = voladj_mom(v, i, cfg_.w_short);
            double vm = voladj_mom(v, i, cfg_.w_mid);
            double vl = voladj_mom(v, i, cfg_.w_long);
            if (std::isnan(vs) || std::isnan(vm) || std::isnan(vl)) continue;
            f_s[s] = vs; f_m[s] = vm; f_l[s] = vl; f_a[s] = vs - vm;
            double dv = avg_dollar_vol(s, i, cfg_.liq_window);
            f_liq[s] = (dv > 0.0) ? std::log(dv) : NAN;
        }
        zscore(f_s); zscore(f_m); zscore(f_l); zscore(f_a); zscore(f_liq);
        for (auto& kv : f_s) {
            const std::string& s = kv.first;
            double zliq = std::isnan(f_liq[s]) ? 0.0 : f_liq[s];
            double liq_pen = cfg_.liq_pen_w * std::max(0.0, -zliq); // penalise below-avg liquidity
            out[s] = cfg_.wt_short*f_s[s] + cfg_.wt_mid*f_m[s]
                   + cfg_.wt_long*f_l[s] + cfg_.wt_accel*f_a[s] - liq_pen;
        }
        return out;
    }

    // ===== breadth / dispersion telemetry (for the allocator, read-only) =====
    double breadth(size_t i) const {
        auto elig = eligible_set(i);
        if (elig.empty()) return 0.0;
        int npos = 0;
        for (auto& s : elig) { double r = trailing_ret(close_.at(s), i, cfg_.w_mid);
            if (!std::isnan(r) && r > 0) ++npos; }
        return (double)npos / elig.size();
    }
    double breadth_latest() const { return days_.empty() ? 0.0 : breadth(days_.size()-1); }
    // smoothed breadth (item 25 regime dead-band input) — averages the raw
    // breadth over the last breadth_smooth days so a single choppy day can't flip
    // the regime. NO price moving-average (standing rule) — this averages a
    // cross-sectional participation ratio, not a price.
    double breadth_smoothed(size_t i) const {
        int k = std::max(1, cfg_.breadth_smooth);
        double sum = 0; int cnt = 0;
        for (int j = 0; j < k && (int)i - j >= 0; ++j) { sum += breadth(i - j); ++cnt; }
        return cnt > 0 ? sum / cnt : 0.0;
    }
    // stateful dead-band regime step: prev = previous bull state. Returns new bull.
    bool regime_step(size_t i, bool prev) const {
        if (!cfg_.breadth_gate) return true;
        double b = breadth_smoothed(i);
        if (cfg_.regime_on > 0.0) {                 // hysteresis dead-band
            if (prev)  return b > cfg_.regime_off;  // stay deployed until it drops through OFF
            else       return b >= cfg_.regime_on;  // deploy only above ON
        }
        return b >= cfg_.breadth_thresh;            // legacy single threshold
    }
    double dispersion(size_t i) const {
        auto elig = eligible_set(i);
        std::vector<double> rs;
        for (auto& s : elig) { double r = trailing_ret(close_.at(s), i, cfg_.w_mid);
            if (!std::isnan(r)) rs.push_back(r); }
        if (rs.size() < 2) return 0.0;
        double m = 0; for (double x : rs) m += x; m /= rs.size();
        double var = 0; for (double x : rs) var += (x-m)*(x-m); var /= rs.size();
        return var > 0 ? std::sqrt(var) : 0.0;
    }
    double dispersion_latest() const { return days_.empty() ? 0.0 : dispersion(days_.size()-1); }

    // ===== SELECTION + SIZING (items 24 corr-pen, 26 core+challenger) ========
    // Greedy diversified pick: repeatedly take the eligible positive-composite
    // name with the best (composite − corr_pen * max_corr_to_already_picked).
    // `held` (optional) applies hysteresis: a held name still in the top-N by
    // composite gets a small tenure bonus so it isn't churned out for a marginal
    // challenger. Fills up to core_k + challenger_k picks; challengers must clear
    // challenger_min_z (cost-separation). Returns picks in selection order.
    std::vector<std::string> select(size_t i, const std::vector<std::string>& elig,
                                    const std::map<std::string,double>& comp,
                                    const std::set<std::string>& held) const {
        // rank pool = eligible with positive composite
        std::vector<std::string> pool;
        for (auto& s : elig) { auto it = comp.find(s); if (it != comp.end() && it->second > 0) pool.push_back(s); }
        // composite rank for the hysteresis "top-N" test
        std::vector<std::pair<double,std::string>> byc;
        for (auto& s : pool) byc.push_back({comp.at(s), s});
        std::sort(byc.begin(), byc.end(), [](auto&a, auto&b){ return a.first > b.first; });
        std::set<std::string> in_top;
        for (int k = 0; k < (int)byc.size() && k < cfg_.hysteresis_top; ++k) in_top.insert(byc[k].second);

        std::vector<std::string> picks;
        std::set<std::string> chosen;
        int want = cfg_.core_k + cfg_.challenger_k;
        while ((int)picks.size() < want) {
            std::string best; double best_adj = -1e18;
            for (auto& s : pool) {
                if (chosen.count(s)) continue;
                double adj = comp.at(s);
                // correlation-concentration penalty vs already-picked names
                double mc = 0.0;
                for (auto& p : picks) mc = std::max(mc, std::fabs(corr(s, p, i)));
                adj -= cfg_.corr_pen_w * mc;
                // hysteresis: keep a held name that is still top-N (small bonus)
                if (held.count(s) && in_top.count(s)) adj += cfg_.challenger_buffer;
                if (adj > best_adj) { best_adj = adj; best = s; }
            }
            if (best.empty()) break;
            // challenger slots (rank >= core_k) must clear the cost-separation bar
            if ((int)picks.size() >= cfg_.core_k && comp.at(best) < cfg_.challenger_min_z) break;
            picks.push_back(best); chosen.insert(best);
        }
        return picks;
    }

    // Target weights (fractions of sleeve NAV; sum <= 1, remainder = CASH).
    // regime: -1 => decide internally from smoothed breadth (no hysteresis memory,
    // used for the startup snapshot / allocator / tests); 0 => forced CASH; 1 =>
    // forced deploy. simulate()/live pass the hysteretic dead-band state.
    std::map<std::string,double> compute_target_weights(size_t i, bool& bull_out,
                                                        const std::set<std::string>& held = {},
                                                        int regime = -1) const {
        std::map<std::string,double> w;
        auto elig = eligible_set(i);
        if (regime == 0)      bull_out = false;
        else if (regime == 1) bull_out = true;
        else                  bull_out = regime_step(i, false);  // stateless snapshot decision
        if (!bull_out || (int)elig.size() < cfg_.min_positive) { bull_out = false; return w; }
        auto comp = composite(i, elig);
        auto picks = select(i, elig, comp, held);
        if ((int)picks.size() < cfg_.min_positive) return w; // not enough conviction -> CASH
        // split into core (first core_k) and challengers (rest)
        std::vector<std::string> core(picks.begin(), picks.begin() + std::min((int)picks.size(), cfg_.core_k));
        std::vector<std::string> chal(picks.begin() + core.size(), picks.end());
        // core: inverse-vol, scaled to core_frac (or full if no challenger deployed)
        double core_target = chal.empty() ? 1.0 : cfg_.core_frac;
        add_inverse_vol(w, core, i, core_target);
        if (!chal.empty()) add_inverse_vol(w, chal, i, 1.0 - cfg_.core_frac);
        return w;
    }

    // ===== FULL SIMULATION over the dense day axis (adaptive + hysteresis) ====
    // Mirrors the live path so the BT reflects real turnover/cost. Returns daily
    // portfolio returns (day, ret) net of per-rebalance turnover cost.
    std::vector<std::pair<int64_t,double>> simulate() const {
        std::vector<std::pair<int64_t,double>> daily;
        if (days_.size() < 2) return daily;
        std::map<std::string,double> wts;
        std::map<std::string,double> entry_vol;   // vol at entry (risk-allowance breach)
        int64_t last_rebal = INT64_MIN/2;
        bool last_bull = false; bool have_last_bull = false;
        for (size_t i = 1; i < days_.size(); ++i) {
            // realise the day's return on yesterday's weights
            double r = 0.0;
            for (auto& kv : wts) {
                double wt = kv.second; if (wt <= 0) continue;
                const auto& v = close_.at(kv.first);
                double a = v[i-1], b = v[i];
                if (!std::isnan(a) && !std::isnan(b) && a > 0) r += wt * (b/a - 1.0);
            }
            daily.push_back({days_[i], r});
            // decide whether to rebalance at close i
            std::set<std::string> held; for (auto& kv : wts) if (kv.second > 0) held.insert(kv.first);
            bool prev_bull = have_last_bull ? last_bull : false;
            bool cur_bull = regime_step(i, prev_bull);   // hysteretic dead-band (no 200DMA)
            int64_t gap = days_[i] - last_rebal;
            bool review = (gap >= cfg_.rebalance_days);
            bool event  = false;
            if (!review && gap >= cfg_.min_rebalance_gap && !held.empty()) {
                auto elig = eligible_set(i);
                auto comp = composite(i, elig);
                // composite rank for the top-N test
                std::vector<std::pair<double,std::string>> byc;
                for (auto& s : elig) { auto it = comp.find(s); if (it != comp.end()) byc.push_back({it->second, s}); }
                std::sort(byc.begin(), byc.end(), [](auto&a, auto&b){ return a.first > b.first; });
                std::set<std::string> in_top; double weakest_held = 1e18;
                for (int k = 0; k < (int)byc.size() && k < cfg_.hysteresis_top; ++k) in_top.insert(byc[k].second);
                for (auto& s : held) { double c = comp.count(s)?comp[s]:-1e9;
                    weakest_held = std::min(weakest_held, c);
                    if (c < 0) event = true;                    // held composite negative
                    if (!in_top.count(s)) event = true;         // held dropped below top-N
                    double cv = realized_vol(close_.at(s), i, cfg_.vol_window);
                    if (entry_vol.count(s) && !std::isnan(cv) && entry_vol[s] > 0 &&
                        cv > entry_vol[s]*cfg_.vol_breach_mult) event = true; // vol breach
                }
                // best non-held challenger beats the weakest held by the buffer
                if (!byc.empty()) { for (auto& pr : byc) { if (held.count(pr.second)) continue;
                    if (pr.first > weakest_held + cfg_.challenger_buffer) event = true; break; } }
                if (have_last_bull && cur_bull != last_bull) event = true; // breadth crossed boundary
            }
            if (have_last_bull && cur_bull != last_bull) review = review || true; // regime flip always acts
            if (!(review || event)) continue;
            bool regime_flip = have_last_bull && (cur_bull != last_bull);
            last_bull = cur_bull; have_last_bull = true;
            bool bull; auto nw = compute_target_weights(i, bull, held, cur_bull ? 1 : 0);
            double turn = 0.0;
            std::set<std::string> allk; for (auto& kv : wts) allk.insert(kv.first);
            for (auto& kv : nw) allk.insert(kv.first);
            for (auto& k : allk) turn += std::fabs((nw.count(k)?nw[k]:0.0) - (wts.count(k)?wts[k]:0.0));
            // NO-TRADE BAND: skip a low-turnover re-tilt (keep weights, no cost).
            // A regime flip (deploy<->cash) always acts. Reset the review clock so
            // we don't re-check every day after a skip.
            if (!regime_flip && turn < cfg_.no_trade_band) { last_rebal = days_[i]; continue; }
            last_rebal = days_[i];
            daily.back().second -= turn * cfg_.cost_bps/10000.0;
            // record entry vol for newly-held names
            entry_vol.clear();
            for (auto& kv : nw) if (kv.second > 0) { double cv = realized_vol(close_.at(kv.first), i, cfg_.vol_window);
                entry_vol[kv.first] = std::isnan(cv) ? 0.0 : cv; }
            wts = nw;
        }
        return daily;
    }

    // ---- live tick path: aggregate to daily close, adaptive rebalance --------
    void on_tick(const std::string& sym, double price, int64_t now_ms) {
        if (price <= 0 || !close_.count(sym)) return;
        int64_t day = now_ms / 86400000LL;
        if (cur_day_ < 0) cur_day_ = day;
        if (day > cur_day_) {
            // carry the last KNOWN dollar-volume forward so the rolling liquidity
            // gate stays valid on live (on_tick has no volume; without this the
            // trailing $-vol window fills with zeros and everything goes ineligible).
            for (auto& kv : last_px_) ingest(kv.first, cur_day_, kv.second,
                                             last_dvol_.count(kv.first) ? last_dvol_[kv.first] : 0.0);
            cur_day_ = day;
            maybe_rebalance(day);
        }
        last_px_[sym] = price;
    }
    void maybe_rebalance(int64_t day) {
        if (days_.empty()) return;
        size_t i = days_.size()-1;
        std::set<std::string> held; for (auto& kv : live_hold_) if (kv.second > 0) held.insert(kv.first);
        bool cur_bull = regime_step(i, have_live_bull_ ? live_last_bull_ : false); // hysteretic dead-band
        int64_t gap = day - live_last_rebal_;
        bool review = (gap >= cfg_.rebalance_days);
        bool event  = false;
        if (!review && gap >= cfg_.min_rebalance_gap && !held.empty()) {
            auto elig = eligible_set(i); auto comp = composite(i, elig);
            std::vector<std::pair<double,std::string>> byc;
            for (auto& s : elig) { auto it = comp.find(s); if (it != comp.end()) byc.push_back({it->second, s}); }
            std::sort(byc.begin(), byc.end(), [](auto&a, auto&b){ return a.first > b.first; });
            std::set<std::string> in_top; for (int k = 0; k < (int)byc.size() && k < cfg_.hysteresis_top; ++k) in_top.insert(byc[k].second);
            for (auto& s : held) { double c = comp.count(s)?comp[s]:-1e9;
                if (c < 0 || !in_top.count(s)) event = true; }
        }
        if (have_live_bull_ && cur_bull != live_last_bull_) { review = true; }
        if (!(review || event)) return;
        bool regime_flip = have_live_bull_ && (cur_bull != live_last_bull_);
        bool bull; auto w = compute_target_weights(i, bull, held, cur_bull ? 1 : 0);
        double turn = 0.0; std::set<std::string> allk;
        for (auto& kv : live_hold_) allk.insert(kv.first); for (auto& kv : w) allk.insert(kv.first);
        for (auto& k : allk) turn += std::fabs((w.count(k)?w[k]:0.0) - (live_hold_.count(k)?live_hold_[k]:0.0));
        live_last_rebal_ = day; live_last_bull_ = cur_bull; have_live_bull_ = true;
        if (!regime_flip && turn < cfg_.no_trade_band) return;  // no-trade band: keep book
        live_hold_.clear(); for (auto& kv : w) live_hold_[kv.first] = kv.second;
        if (on_rebalance_) on_rebalance_(day, w, bull);
    }

    size_t num_days() const { return days_.size(); }
    const XSec2Config& cfg() const { return cfg_; }

private:
    XSec2Config cfg_;
    std::vector<int64_t> days_;
    std::map<int64_t,size_t> day_idx_;
    std::map<std::string,std::vector<double>> close_;
    std::map<std::string,std::vector<double>> dvol_;   // dollar volume aligned to days_
    std::map<std::string,double> last_px_;
    std::map<std::string,double> last_dvol_;   // last known $-vol per symbol (live carry-forward)
    std::map<std::string,double> live_hold_;
    int64_t cur_day_ = -1, live_last_rebal_ = INT64_MIN/2;
    bool have_live_bull_ = false, live_last_bull_ = false;
    RebalanceCallback on_rebalance_;

    void ingest(const std::string& sym, int64_t day, double close, double dvol) {
        if (!close_.count(sym)) { close_[sym] = {}; dvol_[sym] = {}; }
        if (!day_idx_.count(day)) {
            day_idx_[day] = days_.size(); days_.push_back(day);
            for (auto& kv : close_) kv.second.resize(days_.size(), NAN);
            for (auto& kv : dvol_)  kv.second.resize(days_.size(), NAN);
        }
        size_t i = day_idx_[day];
        if (close_[sym].size() < days_.size()) close_[sym].resize(days_.size(), NAN);
        if (dvol_[sym].size()  < days_.size()) dvol_[sym].resize(days_.size(), NAN);
        close_[sym][i] = close;
        dvol_[sym][i]  = dvol;
        if (dvol > 0) last_dvol_[sym] = dvol;
    }

    double avg_dollar_vol(const std::string& sym, size_t i, int n) const {
        auto it = dvol_.find(sym); if (it == dvol_.end()) return NAN;
        const auto& d = it->second;
        if ((int)i < n) return NAN;
        double sum = 0; int cnt = 0;
        for (size_t j = i-n; j < i; ++j) if (!std::isnan(d[j]) && d[j] > 0) { sum += d[j]; ++cnt; }
        return cnt > 0 ? sum/cnt : NAN;   // NAN if no volume info (conservative: illiquid)
    }
    // vol-adjusted momentum = trailing lb-return / realized-vol over the same window
    double voladj_mom(const std::vector<double>& s, size_t i, int lb) const {
        double r = trailing_ret(s, i, lb);
        double v = realized_vol(s, i, lb);
        if (std::isnan(r) || std::isnan(v) || v <= 0) return NAN;
        return r / (v * std::sqrt((double)lb));
    }
    // pairwise Pearson correlation of daily returns over corr_window
    double corr(const std::string& a, const std::string& b, size_t i) const {
        const auto& x = close_.at(a); const auto& y = close_.at(b);
        int n = cfg_.corr_window; if ((int)i < n+1) return 0.0;
        std::vector<double> rx, ry;
        for (size_t j = i-n; j < i; ++j) {
            double xa = x[j-1], xb = x[j], ya = y[j-1], yb = y[j];
            if (std::isnan(xa)||std::isnan(xb)||std::isnan(ya)||std::isnan(yb)||xa<=0||ya<=0) continue;
            rx.push_back(xb/xa-1.0); ry.push_back(yb/ya-1.0);
        }
        if (rx.size() < 10) return 0.0;
        double mx=0,my=0; for (size_t k=0;k<rx.size();++k){mx+=rx[k];my+=ry[k];} mx/=rx.size();my/=ry.size();
        double sxy=0,sxx=0,syy=0;
        for (size_t k=0;k<rx.size();++k){ double dx=rx[k]-mx, dy=ry[k]-my; sxy+=dx*dy; sxx+=dx*dx; syy+=dy*dy; }
        if (sxx<=0||syy<=0) return 0.0;
        return sxy/std::sqrt(sxx*syy);
    }
    void add_inverse_vol(std::map<std::string,double>& w, const std::vector<std::string>& picks,
                         size_t i, double budget) const {
        if (picks.empty() || budget <= 0) return;
        if (cfg_.inverse_vol) {
            std::map<std::string,double> iv; double tot = 0;
            for (auto& s : picks) { double v = realized_vol(close_.at(s), i, cfg_.vol_window);
                double x = (!std::isnan(v) && v > 0) ? 1.0/v : 0.0; iv[s] = x; tot += x; }
            if (tot > 0) for (auto& s : picks) w[s] += budget * iv[s]/tot;
            else         for (auto& s : picks) w[s] += budget / picks.size();
        } else {
            for (auto& s : picks) w[s] += budget / picks.size();
        }
    }
    static void zscore(std::map<std::string,double>& f) {
        std::vector<double> v; for (auto& kv : f) if (!std::isnan(kv.second)) v.push_back(kv.second);
        if (v.size() < 2) { for (auto& kv : f) kv.second = 0.0; return; }
        double m=0; for (double x : v) m += x; m /= v.size();
        double var=0; for (double x : v) var += (x-m)*(x-m); var /= v.size();
        double sd = var > 0 ? std::sqrt(var) : 1.0;
        for (auto& kv : f) kv.second = std::isnan(kv.second) ? 0.0 : (kv.second - m)/sd;
    }
    static double trailing_ret(const std::vector<double>& s, size_t i, int lb) {
        if ((int)i < lb || i >= s.size()) return NAN;
        double a = s[i-lb], b = s[i];
        if (std::isnan(a) || std::isnan(b) || a <= 0) return NAN;
        return b/a - 1.0;
    }
    static double realized_vol(const std::vector<double>& s, size_t i, int n) {
        if ((int)i < n+1) return NAN;
        std::vector<double> rs;
        for (size_t j = i-n; j < i; ++j) { double a=s[j-1], b=s[j];
            if (!std::isnan(a) && !std::isnan(b) && a>0) rs.push_back(b/a-1.0); }
        if ((int)rs.size() < n*0.6) return NAN;
        double m=0; for (double x : rs) m += x; m/=rs.size();
        double var=0; for (double x : rs) var += (x-m)*(x-m); var/=rs.size();
        return var > 0 ? std::sqrt(var) : NAN;
    }
};

} // namespace chimera

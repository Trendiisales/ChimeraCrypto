// ============================================================================
//  CrossSectionalMomentumEngine.hpp
//  The FIRST OOS-validated Chimera edge, built. (2026-06-18)
//
//  Portfolio architecture (NOT per-symbol signals): each rebalance, rank the
//  curated quality-alt universe by trailing 30d return, go LONG the top-K
//  positive-momentum names, inverse-vol weight, and sit in CASH (stablecoin)
//  when the macro regime is bear (BTC < 200d SMA). Long-only spot — the gate is
//  the spot-only "synthetic short" (flat the bear, can't short it).
//
//  Validated (backtest/cross_sectional.py, curated 54-sym quality universe,
//  daily, cost-inclusive 15bp/side) across FOUR independent windows incl the
//  2025 alt-bear holdout:
//     lb30 / top3 / rebal14 / inverse-vol / BTC>200d gate
//     2021 +1596% Sh2.70 | 2023 +324% Sh2.12 | 2024 +159% Sh1.49 | 2025 +39% Sh0.65
//  Naive volume-ranked expansion (103 syms) FAILED 2025 (-56%): the universe
//  must be QUALITY survivors (>=4yr history, liquid), not meme/new-listing junk.
//
//  This header is faithful to cross_sectional.py::run(). simulate() reproduces
//  the Python equity curve (proven by backtest/xsec_faithful.cpp) — the deploy
//  arbiter. The live path (on_tick -> daily-close -> rebalance -> SpotExecutor)
//  reuses the SAME compute_target_weights().
//
//  Warm-seed MANDATORY: needs >=200 daily closes (BTC SMA) before it can gate.
//  seed_daily_close() is fed from REST klines at startup; on_tick maintains the
//  current day thereafter.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace chimera {

struct XSecConfig {
    int    lookback_days  = 30;     // trailing-return ranking window
    int    top_k          = 3;      // long the top-K positive-momentum names
    int    rebalance_days = 14;     // biweekly (low turnover -> cost-robust)
    bool   inverse_vol    = true;   // inverse-vol weight (else equal)
    int    vol_window     = 30;     // realized-vol window for weighting
    int    btc_sma_days   = 200;    // macro regime gate
    bool   macro_gate     = true;   // BTC>SMA -> deploy, else CASH
    double cost_bps       = 15.0;   // per-side turnover cost (SIM only; live uses real fills)
    std::string gate_symbol = "BTC";
};

class CrossSectionalMomentumEngine {
public:
    // Order intent emitted to the live executor (or logged in shadow).
    // weight = target portfolio fraction for `symbol` at this rebalance.
    using RebalanceCallback = std::function<void(int64_t day,
                                                 const std::map<std::string,double>& target_weights,
                                                 bool bull)>;

    explicit CrossSectionalMomentumEngine(XSecConfig cfg = {}) : cfg_(cfg) {}

    void set_universe(const std::vector<std::string>& syms) {
        for (auto& s : syms) if (!close_.count(s)) { close_[s] = {}; }
    }
    void set_rebalance_callback(RebalanceCallback cb) { on_rebalance_ = std::move(cb); }

    // ---- history (warm-seed + live day-rollover both call this) ----
    // day = UTC day index (unix_ms / 86400000). Appends in day order per symbol.
    // The engine keeps a DENSE day axis across all symbols (nan-padded), exactly
    // like cross_sectional.py's daily_close_matrix, so ranking is point-in-time.
    void seed_daily_close(const std::string& sym, int64_t day, double close) {
        ingest(sym, day, close);
    }

    // ===== PURE LOGIC — faithful to cross_sectional.py =====================
    // Target weights at dense-axis index i (point-in-time, no look-ahead).
    std::map<std::string,double> compute_target_weights(size_t i, bool& bull_out) const {
        std::map<std::string,double> w;
        for (auto& kv : close_) w[kv.first] = 0.0;
        bull_out = true;
        if (cfg_.macro_gate) {
            auto it = close_.find(cfg_.gate_symbol);
            double m = (it != close_.end()) ? sma(it->second, i, cfg_.btc_sma_days) : NAN;
            double bc = (it != close_.end() && i < it->second.size()) ? it->second[i] : NAN;
            bull_out = (!std::isnan(m) && !std::isnan(bc) && bc > m);
        }
        if (!bull_out) return w;  // CASH
        std::vector<std::pair<double,std::string>> scores;
        for (auto& kv : close_) {
            double sc = trailing_ret(kv.second, i, cfg_.lookback_days);
            if (std::isnan(sc) || sc <= 0.0) continue;   // long-only: positive momentum only
            scores.push_back({sc, kv.first});
        }
        std::sort(scores.begin(), scores.end(),
                  [](auto&a, auto&b){ return a.first > b.first; });
        std::vector<std::string> picks;
        for (size_t k = 0; k < scores.size() && (int)picks.size() < cfg_.top_k; ++k)
            picks.push_back(scores[k].second);
        if (picks.empty()) return w;
        if (cfg_.inverse_vol) {
            std::map<std::string,double> iv; double tot = 0.0;
            for (auto& s : picks) { double v = realized_vol(close_.at(s), i, cfg_.vol_window);
                double x = (!std::isnan(v) && v > 0) ? 1.0/v : 0.0; iv[s] = x; tot += x; }
            if (tot > 0) for (auto& s : picks) w[s] = iv[s]/tot;
            else         for (auto& s : picks) w[s] = 1.0/picks.size();
        } else {
            for (auto& s : picks) w[s] = 1.0/picks.size();
        }
        return w;
    }

    // Full simulation over the dense day axis — mirrors cross_sectional.py::run().
    // Returns daily portfolio returns (day, ret) with turnover cost applied.
    std::vector<std::pair<int64_t,double>> simulate() const {
        std::vector<std::pair<int64_t,double>> daily;
        if (days_.size() < 2) return daily;
        std::map<std::string,double> wts;
        for (auto& kv : close_) wts[kv.first] = 0.0;
        int64_t last_rebal = INT64_MIN/2;
        for (size_t i = 1; i < days_.size(); ++i) {
            double r = 0.0;
            for (auto& kv : close_) {
                double w = wts[kv.first]; if (w <= 0) continue;
                double a = kv.second[i-1], b = kv.second[i];
                if (!std::isnan(a) && !std::isnan(b) && a > 0) r += w * (b/a - 1.0);
            }
            daily.push_back({days_[i], r});
            if (days_[i] - last_rebal < cfg_.rebalance_days) continue;
            last_rebal = days_[i];
            bool bull; auto nw = compute_target_weights(i, bull);
            double turn = 0.0;
            for (auto& kv : close_) turn += std::fabs(nw[kv.first] - wts[kv.first]);
            daily.back().second -= turn * cfg_.cost_bps/10000.0;
            wts = nw;
        }
        return daily;
    }

    // ---- live tick path: aggregate to daily close, rebalance on the clock ----
    void on_tick(const std::string& sym, double price, int64_t now_ms) {
        if (price <= 0 || !close_.count(sym)) return;
        int64_t day = now_ms / 86400000LL;
        if (cur_day_ < 0) cur_day_ = day;
        if (day > cur_day_) {                 // UTC day rolled — finalize closes
            for (auto& kv : last_px_) ingest(kv.first, cur_day_, kv.second);
            cur_day_ = day;
            maybe_rebalance(day);
        }
        last_px_[sym] = price;
    }

    void maybe_rebalance(int64_t day) {
        if (day - live_last_rebal_ < cfg_.rebalance_days) return;
        if (days_.empty()) return;
        live_last_rebal_ = day;
        bool bull; auto w = compute_target_weights(days_.size()-1, bull);
        if (on_rebalance_) on_rebalance_(day, w, bull);
    }

    size_t num_days() const { return days_.size(); }
    const XSecConfig& cfg() const { return cfg_; }

private:
    XSecConfig cfg_;
    std::vector<int64_t> days_;                       // dense sorted UTC day axis
    std::map<int64_t,size_t> day_idx_;                // day -> axis index
    std::map<std::string,std::vector<double>> close_; // sym -> close aligned to days_ (nan-padded)
    std::map<std::string,double> last_px_;            // live: latest intraday price
    int64_t cur_day_ = -1, live_last_rebal_ = INT64_MIN/2;
    RebalanceCallback on_rebalance_;

    void ingest(const std::string& sym, int64_t day, double close) {
        if (!close_.count(sym)) close_[sym] = {};
        if (!day_idx_.count(day)) {                   // new day: extend axis, nan-pad all
            day_idx_[day] = days_.size(); days_.push_back(day);
            for (auto& kv : close_) kv.second.resize(days_.size(), NAN);
        }
        size_t i = day_idx_[day];
        if (close_[sym].size() < days_.size()) close_[sym].resize(days_.size(), NAN);
        close_[sym][i] = close;
    }

    static double sma(const std::vector<double>& s, size_t i, int n) {
        if ((int)i < n) return NAN;
        double sum = 0; int cnt = 0;
        for (size_t j = i-n; j < i; ++j) if (!std::isnan(s[j])) { sum += s[j]; ++cnt; }
        if (cnt < n*0.8) return NAN;
        return sum/cnt;
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
        for (size_t j = i-n; j < i; ++j) {
            double a = s[j-1], b = s[j];
            if (!std::isnan(a) && !std::isnan(b) && a > 0) rs.push_back(b/a - 1.0);
        }
        if ((int)rs.size() < n*0.6) return NAN;
        double m = 0; for (double x : rs) m += x; m /= rs.size();
        double var = 0; for (double x : rs) var += (x-m)*(x-m); var /= rs.size();
        return var > 0 ? std::sqrt(var) : NAN;
    }
};

} // namespace chimera

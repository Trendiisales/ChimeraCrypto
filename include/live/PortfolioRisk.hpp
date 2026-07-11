#pragma once
// ============================================================================
// PortfolioRisk — item 17 (Phase-3 portfolio unification, 2026-07-11).
//
// BEFORE: "diversification" was assumed from holding several coins, and the only
// correlation proxy was BTC's 200d direction. Cluster caps (where they existed)
// were static symbol lists. Nothing measured the ACTUAL co-movement of the book,
// so a "diversified" set of five alts that all move together carried far more
// risk than the position count implied, and portfolio vol was never targeted.
//
// AFTER: rolling per-symbol returns feed a SHRUNK covariance matrix (Ledoit-Wolf-
// style constant shrink toward the diagonal — stable with short crypto history).
// From it:
//   * portfolio vol from the ACTUAL covariance of the proposed weights,
//   * a VOL-TARGET scalar (scale the whole book to a target portfolio vol),
//   * per-CLUSTER variance-contribution caps computed from real co-movement
//     (a highly-correlated cluster gets scaled down, not treated as diversified),
//   * a crypto-BETA cap (beta of the book to BTC, capped).
//
// These produce a single risk SCALE the allocator multiplies the merged target
// vector by. It never edits a sleeve's signal/exit logic.
// Header-only, no deps, cheaply unit-testable.
// ============================================================================
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <cmath>
#include <algorithm>

namespace chimera {

class PortfolioRisk {
public:
    void configure(int window = 30, double shrink = 0.30) {
        window_ = window < 3 ? 3 : window;
        shrink_ = std::clamp(shrink, 0.0, 1.0);
    }

    // Feed one period return (e.g. daily close-to-close) for a symbol.
    void observe(const std::string& sym, double ret) {
        auto& d = rets_[sym];
        d.push_back(ret);
        while ((int)d.size() > window_) d.pop_front();
    }
    bool ready(const std::string& sym) const {
        auto it = rets_.find(sym);
        return it != rets_.end() && (int)it->second.size() >= 3;
    }

    double mean(const std::string& s) const {
        auto it = rets_.find(s); if (it == rets_.end() || it->second.empty()) return 0.0;
        double m = 0; for (double x : it->second) m += x; return m / it->second.size();
    }
    double variance(const std::string& s) const {
        auto it = rets_.find(s); if (it == rets_.end() || it->second.size() < 2) return 0.0;
        double m = mean(s), v = 0; for (double x : it->second) v += (x-m)*(x-m);
        return v / (it->second.size() - 1);
    }
    double vol(const std::string& s) const { return std::sqrt(variance(s)); }

    // Sample covariance over the overlapping tail of two return series.
    double covariance(const std::string& a, const std::string& b) const {
        auto ia = rets_.find(a), ib = rets_.find(b);
        if (ia == rets_.end() || ib == rets_.end()) return 0.0;
        const auto& da = ia->second; const auto& db = ib->second;
        size_t n = std::min(da.size(), db.size());
        if (n < 2) return 0.0;
        // align on the most-recent n samples
        double ma = 0, mb = 0;
        for (size_t k = 0; k < n; ++k) { ma += da[da.size()-n+k]; mb += db[db.size()-n+k]; }
        ma /= n; mb /= n;
        double c = 0;
        for (size_t k = 0; k < n; ++k)
            c += (da[da.size()-n+k]-ma) * (db[db.size()-n+k]-mb);
        return c / (n - 1);
    }
    double correlation(const std::string& a, const std::string& b) const {
        double va = variance(a), vb = variance(b);
        if (va <= 0 || vb <= 0) return 0.0;
        return covariance(a, b) / std::sqrt(va * vb);
    }

    // Shrunk covariance entry: (1-λ)·sample  +  λ·(diag target).  Off-diagonal
    // shrinks toward 0, the diagonal toward the average variance — the standard
    // constant-correlation / identity shrink that stabilises a short-history
    // crypto covariance.
    double shrunk_cov(const std::string& a, const std::string& b,
                      double avg_var) const {
        double s = covariance(a, b);
        if (a == b) return (1.0 - shrink_) * s + shrink_ * avg_var;
        return (1.0 - shrink_) * s;   // off-diagonal shrinks toward 0
    }

    // Portfolio variance wᵀ Σ w on the shrunk covariance for a weight map (USD or
    // fractional — units cancel in the ratios the allocator uses).
    double portfolio_variance(const std::map<std::string,double>& w) const {
        double avg_var = average_variance(w);
        double v = 0;
        for (auto& i : w) for (auto& j : w)
            v += i.second * j.second * shrunk_cov(i.first, j.first, avg_var);
        return v < 0 ? 0 : v;
    }
    double portfolio_vol(const std::map<std::string,double>& w) const {
        return std::sqrt(portfolio_variance(w));
    }

    // Vol-target scalar: scale the book so its vol ~= target. <=1 (never levers up
    // a long-only spot book). Returns 1.0 when vol can't be estimated yet.
    double vol_target_scale(const std::map<std::string,double>& w,
                            double target_vol) const {
        double pv = portfolio_vol(w);
        if (pv <= 1e-12 || target_vol <= 0) return 1.0;
        return std::min(1.0, target_vol / pv);
    }

    // Per-cluster variance-contribution cap. cluster_of maps symbol->cluster id.
    // Each cluster's share of total portfolio variance is capped at max_frac; an
    // over-contributing (highly-correlated) cluster is scaled down and the book
    // re-measured, iterating a few times. Returns the adjusted weights.
    std::map<std::string,double> cap_clusters(
            std::map<std::string,double> w,
            const std::map<std::string,int>& cluster_of,
            double max_frac, int iters = 12) const {
        for (int it = 0; it < iters; ++it) {
            double V = portfolio_variance(w);
            if (V <= 1e-18) break;
            double avg_var = average_variance(w);
            // marginal contribution to variance: MCV_i = w_i * (Σw)_i
            std::map<int,double> clus_contrib;
            for (auto& i : w) {
                double sigw = 0;
                for (auto& j : w) sigw += j.second * shrunk_cov(i.first, j.first, avg_var);
                int c = cluster_id(cluster_of, i.first);
                clus_contrib[c] += i.second * sigw;
            }
            // find the worst over-cap cluster
            int worst = -1; double worst_frac = max_frac;
            for (auto& kv : clus_contrib) {
                double f = kv.second / V;
                if (f > worst_frac + 1e-9) { worst_frac = f; worst = kv.first; }
            }
            if (worst < 0) break;                 // all clusters within cap
            double s = std::sqrt(max_frac / worst_frac);   // quadratic scale-down
            s = std::clamp(s, 0.05, 0.999);
            for (auto& i : w)
                if (cluster_id(cluster_of, i.first) == worst) i.second *= s;
        }
        return w;
    }

    // Beta of a symbol to a benchmark symbol (e.g. BTC): cov(s,bench)/var(bench).
    double beta(const std::string& sym, const std::string& bench) const {
        double vb = variance(bench);
        if (vb <= 0) return 0.0;
        return covariance(sym, bench) / vb;
    }
    // Weighted book beta to the benchmark.
    double portfolio_beta(const std::map<std::string,double>& w,
                          const std::string& bench) const {
        double tot = 0, b = 0;
        for (auto& kv : w) tot += kv.second;
        if (tot <= 0) return 0.0;
        for (auto& kv : w) b += (kv.second / tot) * beta(kv.first, bench);
        return b;
    }
    // Scale the book down if its beta to the benchmark exceeds a cap.
    double crypto_beta_scale(const std::map<std::string,double>& w,
                             const std::string& bench, double max_beta) const {
        double b = std::fabs(portfolio_beta(w, bench));
        if (b <= max_beta || b <= 1e-12) return 1.0;
        return max_beta / b;
    }

private:
    static int cluster_id(const std::map<std::string,int>& m, const std::string& s) {
        auto it = m.find(s); return it == m.end() ? -1 : it->second;
    }
    double average_variance(const std::map<std::string,double>& w) const {
        double sum = 0; int n = 0;
        for (auto& kv : w) { sum += variance(kv.first); ++n; }
        return n ? sum / n : 0.0;
    }
    int    window_ = 30;
    double shrink_ = 0.30;
    std::map<std::string, std::deque<double>> rets_;
};

// ── Sleeve daily-PnL correlation tracker (item 16 companion) ─────────────────
// Momentum sleeves (XSec / TSMOM / UpJump / RipRider) are one factor; the factor
// cap needs to KNOW how correlated their daily PnL is so genuinely-redundant
// sleeves are not counted as diversification. Pearson over rolling daily returns.
class SleeveCorrelation {
public:
    void observe(const std::string& sleeve, double daily_ret) { risk_.observe(sleeve, daily_ret); }
    double correlation(const std::string& a, const std::string& b) const { return risk_.correlation(a, b); }
    // average pairwise correlation across a set of sleeves (1.0 => fully redundant).
    double mean_pairwise(const std::vector<std::string>& sleeves) const {
        double s = 0; int n = 0;
        for (size_t i = 0; i < sleeves.size(); ++i)
            for (size_t j = i+1; j < sleeves.size(); ++j) { s += risk_.correlation(sleeves[i], sleeves[j]); ++n; }
        return n ? s / n : 0.0;
    }
    PortfolioRisk& risk() { return risk_; }
private:
    PortfolioRisk risk_{};
};

} // namespace chimera

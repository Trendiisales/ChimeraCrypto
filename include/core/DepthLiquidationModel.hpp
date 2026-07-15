// ═══════════════════════════════════════════════════════════════════════════
// DepthLiquidationModel — depth-adjusted full-qty liquidation (CORE/MIMIC §2/§5)
// ═══════════════════════════════════════════════════════════════════════════
// Phase-2 primitive. The spec's authoritative BE/protection math requires the
// price at which the FULL quantity could actually be sold into current bid
// depth — not last price, midpoint or best bid alone (§2). Nothing in the book
// walked the ladder before this.
//
// DATA: Binance-Vision perp `bookDepth` daily dumps (data/bookdepth_perp/).
//   rows: `timestamp,percentage,depth,notional`
//   `percentage` ∈ {-5,-4,-3,-2,-1,+1,+2,+3,+4,+5} — CUMULATIVE bid(neg)/ask(pos)
//   `depth`    = cumulative base qty available within that % band of mid
//   `notional` = cumulative USD (Σ price·qty) for that slice → notional/depth is
//                the VWAP of selling (bid) / buying (ask) everything down/up to
//                that band edge. ~30s cadence.
//   PERP book (spot has no free depth); BTC/ETH/XRP perp is deepest/tightest-
//   basis → a legitimate proxy (residual: perp≠spot, ~30s cadence). Stated in
//   CORE_MIMIC_BUILD_PLAN.md "DATA REALITY".
//
// MODEL (linear-order-book reconstruction):
//   The file gives only 5 cumulative points per side + the implied mid. We
//   reconstruct a piecewise-linear marginal price so tiny Q → ~0 slip (the
//   physically-real behaviour; a flat step-function would falsely charge the
//   whole band-1 VWAP on a $10k clip). For each side, anchor price p0=mid at
//   qty 0; segment k avg price m_k=(N_k−N_{k-1})/(Q_k−Q_{k-1}); boundary price
//   p_k=2·m_k−p_{k-1} (linear-in-qty price ⇒ segment integral == cumulative
//   notional, exact by construction). Clamp p_k monotone away from mid (noise
//   guard). Q beyond the ±5% book charges the deepest marginal p_5 flat and
//   sets beyond_book=true (conservative floor, flagged).
//
// mid = 0.5·(bidVWAP(−1) + askVWAP(+1)) — the two innermost VWAPs bracket mid;
// the file carries no top-of-book, this is the best available reference and is
// used only to express slip in bps (proceeds themselves need no reference).
//
// Everything SHADOW; forward-measured live once the executor feeds real fills.
#pragma once

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace chimera {

// One ~30s depth snapshot. Cumulative qty/notional at |band| 1..5 (index 1..5;
// [0] unused so band index == array index).
struct DepthSnapshot {
    int64_t ts_ms = 0;
    std::array<double,6> bid_q{};   // cumulative base qty, bid side (neg bands)
    std::array<double,6> bid_n{};   // cumulative USD notional, bid side
    std::array<double,6> ask_q{};
    std::array<double,6> ask_n{};
    double mid = 0.0;
    bool   valid = false;           // all 10 bands present & mid > 0

    // Proceeds (USD) from selling Q base into the bid ladder. Linear-OB walk.
    // Sets beyond_book=true if Q exceeds the −5% cumulative depth.
    double sell_proceeds(double Q, bool* beyond_book = nullptr) const {
        return walk_(Q, bid_q, bid_n, /*sell=*/true, beyond_book);
    }
    // Cost (USD) to buy Q base from the ask ladder.
    double buy_cost(double Q, bool* beyond_book = nullptr) const {
        return walk_(Q, ask_q, ask_n, /*sell=*/false, beyond_book);
    }
    // Slip of a full-qty SELL vs mid, in bps (>=0). 0 if Q<=0 or invalid.
    double sell_slip_bps(double Q, bool* beyond_book = nullptr) const {
        if (Q <= 0 || !valid || mid <= 0) return 0.0;
        double proc = sell_proceeds(Q, beyond_book);
        double vwap = proc / Q;
        return (mid - vwap) / mid * 1e4;
    }
    // Slip of a full-qty BUY vs mid, in bps (>=0).
    double buy_slip_bps(double Q, bool* beyond_book = nullptr) const {
        if (Q <= 0 || !valid || mid <= 0) return 0.0;
        double cost = buy_cost(Q, beyond_book);
        double vwap = cost / Q;
        return (vwap - mid) / mid * 1e4;
    }

private:
    // Walk the cumulative (Q_k,N_k) ladder with a linear-in-qty price per
    // segment, anchored at (0, mid). sell=true → prices fall; false → rise.
    double walk_(double Q, const std::array<double,6>& cq,
                 const std::array<double,6>& cn, bool sell, bool* beyond) const {
        if (beyond) *beyond = false;
        if (Q <= 0) return 0.0;
        double p_prev = mid, q_prev = 0.0, n_prev = 0.0;
        for (int k = 1; k <= 5; ++k) {
            double Qk = cq[k], Nk = cn[k];
            if (Qk <= q_prev) continue;                 // degenerate band, skip
            double m_k = (Nk - n_prev) / (Qk - q_prev); // segment avg price
            double p_k = 2.0 * m_k - p_prev;            // linear boundary price
            // monotone clamp (noise guard): sells never rise, buys never fall
            if (sell  && p_k > p_prev) p_k = p_prev;
            if (!sell && p_k < p_prev) p_k = p_prev;
            if (Q <= Qk) {                              // fills inside this band
                double frac = (Q - q_prev) / (Qk - q_prev);
                double p_at = p_prev + (p_k - p_prev) * frac;
                double partial = (Q - q_prev) * (p_prev + p_at) * 0.5;
                return n_prev + partial;
            }
            p_prev = p_k; q_prev = Qk; n_prev = Nk;
        }
        // Beyond the ±5% book: charge the deepest marginal price flat (floor).
        if (beyond) *beyond = true;
        return n_prev + (Q - q_prev) * p_prev;
    }
};

// A day-or-more of snapshots for one symbol, sorted by ts; nearest-ts lookup.
class DepthBook {
public:
    // Parse "YYYY-MM-DD HH:MM:SS" (UTC) → epoch ms. No TZ dep.
    static int64_t parse_ts(const std::string& s) {
        int Y,Mo,D,H,Mi,Se;
        if (std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &Y,&Mo,&D,&H,&Mi,&Se) != 6)
            return 0;
        static const int cum[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
        int64_t days = (int64_t)(Y-1970)*365 + (Y-1969)/4; // leap days since 1970
        days += cum[Mo-1];
        if (Mo > 2 && (Y%4==0 && (Y%100!=0 || Y%400==0))) days += 1;
        days += (D-1);
        return (((days*24 + H)*60 + Mi)*60 + Se) * 1000LL;
    }

    // Load every ETH/XRP-style file matching prefix in dir. Returns snapshot count.
    // Files are one-day dumps with 10 rows per timestamp (bands -5..-1,1..5).
    size_t load_dir(const std::string& dir, const std::string& prefix) {
        std::vector<std::string> files = list_(dir, prefix);
        for (const auto& f : files) load_file_(f);
        std::sort(snaps_.begin(), snaps_.end(),
                  [](const DepthSnapshot& a, const DepthSnapshot& b){ return a.ts_ms < b.ts_ms; });
        return snaps_.size();
    }
    size_t load_file(const std::string& path) { size_t n0=snaps_.size(); load_file_(path);
        std::sort(snaps_.begin(), snaps_.end(),
                  [](const DepthSnapshot& a, const DepthSnapshot& b){ return a.ts_ms < b.ts_ms; });
        return snaps_.size()-n0; }

    const std::vector<DepthSnapshot>& snaps() const { return snaps_; }
    bool empty() const { return snaps_.empty(); }

    // Nearest snapshot at/around ts_ms (binary search). Returns nullptr if the
    // gap to the closest snapshot exceeds max_gap_ms (default 5 min → stale).
    const DepthSnapshot* nearest(int64_t ts_ms, int64_t max_gap_ms = 300000) const {
        if (snaps_.empty()) return nullptr;
        auto it = std::lower_bound(snaps_.begin(), snaps_.end(), ts_ms,
            [](const DepthSnapshot& s, int64_t t){ return s.ts_ms < t; });
        const DepthSnapshot* best = nullptr; int64_t bd = max_gap_ms + 1;
        auto consider = [&](std::vector<DepthSnapshot>::const_iterator c){
            if (c < snaps_.begin() || c >= snaps_.end()) return;
            int64_t d = std::llabs(c->ts_ms - ts_ms);
            if (d < bd) { bd = d; best = &*c; } };
        if (it != snaps_.end()) consider(it);
        if (it != snaps_.begin()) consider(it - 1);
        return (best && bd <= max_gap_ms) ? best : nullptr;
    }

private:
    std::vector<DepthSnapshot> snaps_;

    static std::vector<std::string> list_(const std::string& dir, const std::string& prefix) {
        // Portable: shell out to ls (data dirs are small, one-time load).
        std::vector<std::string> out;
        std::string cmd = "ls " + dir + "/" + prefix + "* 2>/dev/null";
        FILE* p = popen(cmd.c_str(), "r");
        if (!p) return out;
        char line[4096];
        while (fgets(line, sizeof(line), p)) {
            std::string s(line); while (!s.empty() && (s.back()=='\n'||s.back()=='\r')) s.pop_back();
            if (!s.empty()) out.push_back(s);
        }
        pclose(p);
        return out;
    }

    void load_file_(const std::string& path) {
        std::ifstream f(path);
        if (!f) { std::fprintf(stderr, "DepthBook: cannot open %s\n", path.c_str()); return; }
        std::string ln; std::getline(f, ln); // header
        DepthSnapshot cur; std::string cur_ts_str; int bands_seen = 0;
        auto finalize = [&](){
            if (bands_seen == 10) {
                double bv1 = cur.bid_q[1] > 0 ? cur.bid_n[1]/cur.bid_q[1] : 0;
                double av1 = cur.ask_q[1] > 0 ? cur.ask_n[1]/cur.ask_q[1] : 0;
                if (bv1 > 0 && av1 > 0) { cur.mid = 0.5*(bv1+av1); cur.valid = true; snaps_.push_back(cur); }
            }
        };
        while (std::getline(f, ln)) {
            if (ln.empty()) continue;
            // split first 4 fields: ts (has a space+comma structure) then band,depth,notional
            // format: "2025-05-10 00:00:08,-1,37580.245,59094959.115"
            size_t c1 = ln.find(',');
            if (c1 == std::string::npos) continue;
            std::string ts = ln.substr(0, c1);
            size_t c2 = ln.find(',', c1+1);
            size_t c3 = ln.find(',', c2+1);
            if (c2==std::string::npos || c3==std::string::npos) continue;
            int    band = std::atoi(ln.substr(c1+1, c2-c1-1).c_str());
            double dep  = std::atof(ln.substr(c2+1, c3-c2-1).c_str());
            double ntl  = std::atof(ln.substr(c3+1).c_str());
            if (ts != cur_ts_str) {
                finalize();
                cur = DepthSnapshot{}; cur.ts_ms = parse_ts(ts); cur_ts_str = ts; bands_seen = 0;
            }
            int a = std::abs(band);
            if (a >= 1 && a <= 5) {
                if (band < 0) { cur.bid_q[a] = dep; cur.bid_n[a] = ntl; }
                else          { cur.ask_q[a] = dep; cur.ask_n[a] = ntl; }
                bands_seen++;
            }
        }
        finalize();
    }
};

} // namespace chimera

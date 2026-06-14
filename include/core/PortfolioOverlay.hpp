// ============================================================================
// PortfolioOverlay.hpp — cross-sectional + vol-scaled sizing multiplier
//
// AUDIT-2026 addition. Two overlays composed into one per-symbol multiplier:
//
//   1) Cross-sectional momentum (XSec):
//        Liu/Tsyvinski/Wu NBER w25882; ACFR working paper 2024.
//        Rank tradable symbols (BTC..ARB) by 28-day cumulative return.
//        Top-3 -> 1.5x sizing.  Bottom-3 -> 0.5x.  Middle-6 -> 1.0x.
//        Recompute hourly (cheap); ranking effectively rebalances on
//        bar-close entries.
//
//   2) Volatility scaling (Barroso/Santa-Clara, AQR TSMOM):
//        target_vol / realized_vol_20d, clamped to [0.5, 1.5].
//        Stabilises position risk across regimes.
//
// Final multiplier = clamp(xsec_mult * vol_mult, 0.25, 2.0).
//
// Data source: per-symbol DAILY closes deque. Warm-started from
// data/klines_spot/{SYMBOL}_1h_extended.csv (we sample every 24th 1h bar to
// build daily series). Live updated by tick stream (on_tick).
//
// EdgeEngine.hpp untouched. This module is a portfolio overlay only — it
// reads spot prices and returns a sizing multiplier; never queries engine
// internals.
// ============================================================================
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "SymbolIndex.hpp"

namespace chimera {

class PortfolioOverlay {
public:
    // How many tradeables (BTC..ARB). PEPE/WIF/FET/ONDO/TIA are display-only.
    static constexpr int N_TRADEABLE = 12;
    static constexpr int LOOKBACK_DAYS_XSEC = 28;
    static constexpr int LOOKBACK_DAYS_VOL  = 20;
    static constexpr int HIST_DAYS = 64;  // keep a buffer beyond max lookback

    static constexpr double TARGET_VOL_ANNUAL = 0.60;  // 60% annualised target
    static constexpr double VOL_CLAMP_LO = 0.5, VOL_CLAMP_HI = 1.5;
    static constexpr double XSEC_TOP_MULT = 1.5;
    static constexpr double XSEC_BOT_MULT = 0.5;
    static constexpr double FINAL_LO = 0.25, FINAL_HI = 2.0;

    PortfolioOverlay() {
        for (auto& m : final_mult_) m.store(1.0);
        for (auto& r : ret28d_)     r.store(0.0);
        for (auto& v : real_vol_)   v.store(0.0);
        for (auto& rk : rank_)      rk.store(-1);
    }

    // Warm-start: read 1h CSVs, sample daily close = close at hour 23:00 UTC.
    // CSV row: open_time_ms,open,high,low,close,volume,close_time_ms,...
    // Daily close ≡ row whose open_time_ms % 86400000 == 23*3600*1000.
    // Returns # symbols warmed.
    int warm_start_from_csv(const std::string& dir) {
        int warmed = 0;
        for (int i = 0; i < N_TRADEABLE; ++i) {
            std::string upper = std::string(sym_short(i));
            for (auto& c : upper) c = (char)std::toupper((unsigned char)c);
            std::string path = dir + "/" + upper + "USDT_1h_extended.csv";
            std::ifstream f(path);
            if (!f.is_open()) {
                std::fprintf(stderr, "[OVERLAY] warm-start: cannot open %s\n", path.c_str());
                std::fflush(stderr);
                continue;
            }
            std::string line;
            std::getline(f, line);  // header
            auto& s = series_[i];
            s.daily_closes.clear();
            constexpr int64_t DAY_MS = 86400000;
            constexpr int64_t DAILY_CLOSE_MOD = 23LL * 3600LL * 1000LL;  // 23:00 UTC
            while (std::getline(f, line)) {
                if (line.empty()) continue;
                int64_t ts = 0;
                double close = 0.0;
                int field = 0;
                std::size_t p = 0;
                while (p < line.size() && field < 5) {
                    std::size_t q = line.find(',', p);
                    if (q == std::string::npos) q = line.size();
                    std::string tok = line.substr(p, q - p);
                    if (field == 0)      ts    = std::strtoll(tok.c_str(), nullptr, 10);
                    else if (field == 4) close = std::strtod(tok.c_str(), nullptr);
                    p = q + 1;
                    ++field;
                }
                if (ts > 0 && close > 0.0 && (ts % DAY_MS) == DAILY_CLOSE_MOD) {
                    s.daily_closes.push_back(close);
                    if ((int)s.daily_closes.size() > HIST_DAYS) s.daily_closes.pop_front();
                }
            }
            if (!s.daily_closes.empty()) {
                s.last_close_seen = s.daily_closes.back();
                ++warmed;
            }
        }
        recompute_();
        std::printf("[OVERLAY] warm-start: %d/%d symbols seeded (HIST_DAYS=%d)\n",
                    warmed, N_TRADEABLE, HIST_DAYS);
        // Dump first-pass ranking + multipliers so you can eyeball the overlay.
        for (int i = 0; i < N_TRADEABLE; ++i) {
            std::printf("[OVERLAY]   %-5s  ret28d=%+7.2f%%  vol20d=%6.4f  rank=%2d  mult=%.2fx\n",
                        sym_short(i),
                        ret_28d(i) * 100.0,
                        realised_vol(i),
                        rank(i),
                        multiplier_for(i));
        }
        std::fflush(stdout);
        return warmed;
    }

    // Called from spot tick callback. Maintains daily-close deque per symbol.
    void on_tick(int symbol_id, double px, int64_t now_ms) {
        if (symbol_id < 0 || symbol_id >= N_TRADEABLE) return;
        if (px <= 0.0) return;
        constexpr int64_t DAY_MS = 86400000;
        int64_t day_id = now_ms / DAY_MS;

        std::lock_guard<std::mutex> lk(mtx_);
        auto& s = series_[symbol_id];
        s.last_close_seen = px;
        if (s.last_day_id < 0) {
            s.last_day_id = day_id;
            return;
        }
        if (day_id > s.last_day_id) {
            // Day rolled — push prior day's last seen close as daily close.
            // (Sample boundary is approximate; we just want a daily-cadence series.)
            s.daily_closes.push_back(px);
            if ((int)s.daily_closes.size() > HIST_DAYS) s.daily_closes.pop_front();
            s.last_day_id = day_id;
            // Recompute opportunistically; cheap with N=12.
            recompute_();
        }
    }

    // Final sizing multiplier for one symbol. 1.0 if not warm.
    double multiplier_for(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= N_TRADEABLE) return 1.0;
        return final_mult_[symbol_id].load();
    }

    // Diagnostics
    double ret_28d(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= N_TRADEABLE) return 0.0;
        return ret28d_[symbol_id].load();
    }
    double realised_vol(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= N_TRADEABLE) return 0.0;
        return real_vol_[symbol_id].load();
    }
    int rank(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= N_TRADEABLE) return -1;
        return rank_[symbol_id].load();
    }

    // Emit JSON fragment (no surrounding braces, comma-separated entries).
    void to_json(std::ostringstream& js) const {
        js << "\"portfolio_overlay\":{";
        for (int i = 0; i < N_TRADEABLE; ++i) {
            if (i > 0) js << ",";
            js << "\"" << sym_short(i) << "\":{"
               << "\"mult\":" << multiplier_for(i)
               << ",\"ret28d\":" << ret_28d(i)
               << ",\"vol20d\":" << realised_vol(i)
               << ",\"rank\":"  << rank(i)
               << "}";
        }
        js << "}";
    }

private:
    struct Series {
        std::deque<double> daily_closes;
        int64_t last_day_id = -1;
        double  last_close_seen = 0.0;
    };

    std::array<Series, N_TRADEABLE> series_{};
    std::array<std::atomic<double>, N_TRADEABLE> final_mult_{};
    std::array<std::atomic<double>, N_TRADEABLE> ret28d_{};
    std::array<std::atomic<double>, N_TRADEABLE> real_vol_{};
    std::array<std::atomic<int>,    N_TRADEABLE> rank_{};
    mutable std::mutex mtx_;

    // Caller must hold mtx_. Non-const because std::atomic<T>::store() is non-const.
    void recompute_() {
        // 1. Compute 28d return per symbol.
        std::array<double, N_TRADEABLE> rets{};
        std::array<bool,   N_TRADEABLE> warm{};
        for (int i = 0; i < N_TRADEABLE; ++i) {
            const auto& s = series_[i];
            const int n = (int)s.daily_closes.size();
            if (n < LOOKBACK_DAYS_XSEC + 1) { warm[i] = false; rets[i] = 0.0; continue; }
            double now_px = s.daily_closes[n - 1];
            double then_px = s.daily_closes[n - 1 - LOOKBACK_DAYS_XSEC];
            if (then_px <= 0.0) { warm[i] = false; rets[i] = 0.0; continue; }
            rets[i] = (now_px - then_px) / then_px;
            warm[i] = true;
        }

        // 2. Rank warm symbols by return (descending).
        std::vector<int> warm_ids;
        warm_ids.reserve(N_TRADEABLE);
        for (int i = 0; i < N_TRADEABLE; ++i) if (warm[i]) warm_ids.push_back(i);
        std::sort(warm_ids.begin(), warm_ids.end(),
                  [&](int a, int b) { return rets[a] > rets[b]; });

        // 3. Assign XSec multiplier by tercile (top-3 / mid / bot-3).
        std::array<double, N_TRADEABLE> xsec{};
        std::array<int,    N_TRADEABLE> rk{};
        for (auto& x : xsec) x = 1.0;
        for (auto& r : rk) r = -1;
        const int nw = (int)warm_ids.size();
        for (int p = 0; p < nw; ++p) {
            int id = warm_ids[p];
            rk[id] = p + 1;
            if (nw >= 6) {
                if (p < 3) xsec[id] = XSEC_TOP_MULT;
                else if (p >= nw - 3) xsec[id] = XSEC_BOT_MULT;
                else xsec[id] = 1.0;
            }
        }

        // 4. Compute 20d realised vol (stddev of daily log returns) per symbol;
        //    map to vol multiplier = clamp(target_daily_vol / realised, lo, hi).
        // target_daily_vol = TARGET_VOL_ANNUAL / sqrt(365).
        const double target_daily_vol = TARGET_VOL_ANNUAL / std::sqrt(365.0);
        std::array<double, N_TRADEABLE> volm{};
        std::array<double, N_TRADEABLE> rvol{};
        for (auto& v : volm) v = 1.0;
        for (int i = 0; i < N_TRADEABLE; ++i) {
            const auto& s = series_[i];
            const int n = (int)s.daily_closes.size();
            if (n < LOOKBACK_DAYS_VOL + 2) { rvol[i] = 0.0; continue; }
            double mean = 0.0;
            std::vector<double> rs;
            rs.reserve(LOOKBACK_DAYS_VOL);
            for (int k = 0; k < LOOKBACK_DAYS_VOL; ++k) {
                double c0 = s.daily_closes[n - 2 - k];
                double c1 = s.daily_closes[n - 1 - k];
                if (c0 <= 0.0 || c1 <= 0.0) continue;
                double lr = std::log(c1 / c0);
                rs.push_back(lr);
                mean += lr;
            }
            if (rs.size() < 5) continue;
            mean /= (double)rs.size();
            double ss = 0.0;
            for (double lr : rs) { double d = lr - mean; ss += d * d; }
            double sd = std::sqrt(ss / (double)(rs.size() - 1));
            rvol[i] = sd;
            if (sd > 1e-9) {
                double m = target_daily_vol / sd;
                if (m < VOL_CLAMP_LO) m = VOL_CLAMP_LO;
                if (m > VOL_CLAMP_HI) m = VOL_CLAMP_HI;
                volm[i] = m;
            }
        }

        // 5. Compose, publish.
        for (int i = 0; i < N_TRADEABLE; ++i) {
            double m = xsec[i] * volm[i];
            if (m < FINAL_LO) m = FINAL_LO;
            if (m > FINAL_HI) m = FINAL_HI;
            final_mult_[i].store(m);
            ret28d_[i].store(rets[i]);
            real_vol_[i].store(rvol[i]);
            rank_[i].store(rk[i]);
        }
    }
};

}  // namespace chimera

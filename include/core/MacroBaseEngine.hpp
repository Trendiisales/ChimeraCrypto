// S55: MacroBaseEngine — the bull-beta CORE that catches explosions (you're already
// long when they hit). 200d-MA timed buy-and-hold of a major basket with:
//   enter  : BTC > 200d-MA * (1+enter_band)   (confirm the bull, kill false breakouts)
//   exit   : BTC < 200d-MA * (1-exit_band)     (macro-bear flip)  OR
//   hard DD: flatten if NAV drops dd_stop from peak (catches fast crashes the slow MA misses)
// Validated (11yr BTC/ETH/SOL): hyst 3% + hard-DD 20% -> maxDD ~22-26% (vs buy-hold
// 77-97%) while keeping 10-18x. NAV tracker (shadow): no orders, marks the basket
// to market. Equal-weight basket, alloc fraction in basket, rest cash.
#pragma once
#include <vector>
#include <string>
#include <cstdio>

namespace chimera {

class MacroBaseEngine {
public:
    struct Config {
        std::vector<std::string> symbols;   // basket (equal-weight)
        double alloc      = 0.95;           // fraction deployed into the basket when invested
        double enter_band = 0.03;           // enter when BTC > MA*(1+enter_band)
        double exit_band  = 0.03;           // exit  when BTC < MA*(1-exit_band)
        double dd_stop    = 0.20;           // flatten if NAV -dd_stop from peak
    };
    explicit MacroBaseEngine(const Config& c) : cfg_(c), entry_(c.symbols.size(), 0.0) {}

    const Config& cfg() const { return cfg_; }
    bool   invested()  const { return invested_; }
    bool   dd_locked() const { return dd_locked_; }
    double nav()       const { return nav_; }
    double peak()      const { return peak_nav_; }
    int    flips()     const { return flips_; }
    double ret_bp()    const { return (nav_ - 1.0) * 1e4; }

    // spot[i] aligned to cfg_.symbols; btc_spot/btc_ma drive the macro signal.
    void update(double btc_spot, double btc_ma, const std::vector<double>& spot, long long ts_ms) {
        (void)ts_ms;
        if (btc_ma <= 0 || btc_spot <= 0 || spot.size() != cfg_.symbols.size()) return;
        for (double s : spot) if (s <= 0) return;     // need every basket price live

        bool want_in  = btc_spot > btc_ma * (1.0 + cfg_.enter_band);
        bool want_out = btc_spot < btc_ma * (1.0 - cfg_.exit_band);

        if (invested_) {
            // equal-weight basket return multiple since entry
            double r = 0.0, w = 1.0 / (double)spot.size();
            for (size_t i = 0; i < spot.size(); ++i) r += w * (spot[i] / entry_[i]);
            nav_ = nav_at_entry_ * ((1.0 - cfg_.alloc) + cfg_.alloc * r);
            if (nav_ > peak_nav_) peak_nav_ = nav_;
            double dd = (peak_nav_ > 0) ? (peak_nav_ - nav_) / peak_nav_ : 0.0;
            if (dd >= cfg_.dd_stop || want_out) {
                invested_ = false; flips_++;
                if (dd >= cfg_.dd_stop) {
                    dd_locked_ = true;
                    std::printf("[MACRO-BASE] HARD-DD EXIT dd=%.1f%% nav=%.4f -> cash (re-arm on fresh bull)\n", 100*dd, nav_);
                } else {
                    std::printf("[MACRO-BASE] MACRO-BEAR EXIT nav=%.4f -> cash\n", nav_);
                }
                std::fflush(stdout);
            }
        } else {
            // dd-locked: wait for a fresh bull signal before re-entering
            if (dd_locked_) { if (want_in) dd_locked_ = false; else return; }
            if (want_in) {
                invested_ = true; nav_at_entry_ = nav_; peak_nav_ = nav_; flips_++;
                for (size_t i = 0; i < spot.size(); ++i) entry_[i] = spot[i];
                std::printf("[MACRO-BASE] BULL ENTRY nav=%.4f -> %zu-asset basket @ %.0f%% alloc\n",
                            nav_, spot.size(), 100*cfg_.alloc);
                std::fflush(stdout);
            }
        }
    }

private:
    Config cfg_;
    std::vector<double> entry_;
    bool   invested_ = false, dd_locked_ = false;
    double nav_ = 1.0, nav_at_entry_ = 1.0, peak_nav_ = 1.0;
    int    flips_ = 0;
};

} // namespace chimera

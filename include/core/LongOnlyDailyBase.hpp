// ============================================================================
//  LongOnlyDailyBase.hpp  —  Phase-6 shared base for the NEW long-only daily
//  spot strategy families (trend-pullback/reclaim, compression breakout,
//  bull-regime mean-reversion).  Header-only, dep-free (stdlib only) so the
//  backtest harness + unit tests include it directly.
//
//  HARD constraints (operator, standing crypto rules):
//    * LONG-ONLY SPOT, NO SHORTS.
//    * NO 200DMA ANYWHERE — the REGIME gate is a BREADTH participation ratio
//      (share of the eligible universe with a positive trailing return), never a
//      price moving average.  (Per-coin EMAs used *inside* a family's own entry
//      logic — e.g. "pull back to a rising EMA" — are the strategy definition,
//      not a 200-day regime bull-gate, and are allowed.  None is a 200DMA.)
//
//  Design: a template-method base.  It stores per-symbol daily OHLCV on a dense
//  union day axis (NaN before a coin lists), precomputes a standard indicator
//  set once per engine instance, runs the SAME per-day portfolio step for both
//  the batch simulate() (the BACKTEST_TRUTH gate) and the live on_tick shadow
//  path, and calls two virtual hooks each family overrides:
//     entry_signal(sym,i)     — is there a fresh long entry at close i?
//     exit_signal(sym,i,pos)  — should an open long be closed at close i?
//  Position sizing, the breadth regime gate, max-positions, per-name cap,
//  inverse-vol option and turnover cost live HERE (identical across families).
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

struct LODConfig {
    // ---- portfolio construction (shared) -----------------------------------
    int    max_positions   = 8;      // cap on simultaneous longs
    double per_name_cap    = 0.20;   // max fraction of sleeve NAV per name
    bool   inverse_vol     = true;   // size by inverse realized vol (else equal)
    int    vol_window      = 20;

    // ---- regime gate (BREADTH ONLY — NO 200DMA) ----------------------------
    bool   breadth_gate    = true;
    double breadth_thresh  = 0.40;   // deploy new entries above this smoothed breadth
    int    breadth_smooth  = 5;      // days to average breadth (kills single-day flips)
    int    breadth_lb      = 30;     // trailing return lookback for participation

    // ---- eligibility (point-in-time, no survivor list) ---------------------
    int    min_history     = 120;    // listing-age proxy (contiguous real closes)
    int    liq_window      = 30;
    double min_dollar_vol  = 2.0e6;  // avg daily $-vol floor (spot liquidity)

    // ---- standard indicator windows (precomputed once) ---------------------
    int    ema_fast_n      = 20;
    int    ema_slow_n      = 50;
    int    atr_n           = 14;
    int    rsi_n           = 14;
    int    bb_n            = 20;
    double bb_k            = 2.0;
    int    vol_avg_n       = 20;     // avg base-volume window (breakout confirmation)

    double cost_bps        = 15.0;   // per-side turnover cost (SIM only; live=real fills)
    int    rebalance_gap   = 1;      // min days between portfolio steps (1 = daily)
};

class LongOnlyDailyBase {
public:
    struct Position {
        double entry_price = 0.0;
        double entry_atr   = 0.0;
        double entry_vol   = 0.0;
        double highest_close = 0.0;  // for trailing runners
        int    bars_held   = 0;
        size_t entry_i     = 0;
    };
    using RebalanceCallback = std::function<void(int64_t day,
                                    const std::map<std::string,double>& target_weights,
                                    bool bull)>;

    explicit LongOnlyDailyBase(LODConfig cfg = {}) : cfg_(cfg) {}
    virtual ~LongOnlyDailyBase() = default;

    void set_universe(const std::vector<std::string>& syms) {
        for (auto& s : syms) if (!o_.count(s)) { o_[s]={}; h_[s]={}; l_[s]={}; c_[s]={}; v_[s]={}; }
    }
    void set_rebalance_callback(RebalanceCallback cb) { on_rebalance_ = std::move(cb); }

    // day = unix_ms / 86400000.  v = base volume ($-vol = close*v).
    void seed_daily(const std::string& sym, int64_t day,
                    double o, double hi, double lo, double cl, double vol) {
        ingest(sym, day, o, hi, lo, cl, vol); dirty_ = true;
    }
    void seed_daily_close(const std::string& sym, int64_t day, double cl) {
        ingest(sym, day, cl, cl, cl, cl, 0.0); dirty_ = true;
    }

    size_t num_days() const { return days_.size(); }
    const LODConfig& cfg() const { return cfg_; }

    // ---- eligibility (item 27: point-in-time, no survivor list) ------------
    bool eligible(const std::string& sym, size_t i) const {
        auto it = c_.find(sym);
        if (it == c_.end() || i >= it->second.size()) return false;
        const auto& s = it->second;
        if (std::isnan(s[i]) || s[i] <= 0) return false;
        int lo = (int)i - cfg_.min_history; if (lo < 0) return false;
        int nn = 0; for (int j = lo; j < (int)i; ++j) if (!std::isnan(s[j]) && s[j] > 0) ++nn;
        if (nn < 0.80 * cfg_.min_history) return false;
        double dv = avg_dollar_vol(sym, i, cfg_.liq_window);
        if (cfg_.min_dollar_vol > 0.0 && (std::isnan(dv) || dv < cfg_.min_dollar_vol)) return false;
        return true;
    }

    // ---- breadth regime (NO 200DMA) ----------------------------------------
    double breadth(size_t i) const {
        int npos = 0, n = 0;
        for (auto& kv : c_) { if (!eligible(kv.first, i)) continue; ++n;
            double r = ret(kv.first, i, cfg_.breadth_lb);
            if (!std::isnan(r) && r > 0) ++npos; }
        return n > 0 ? (double)npos / n : 0.0;
    }
    double breadth_smoothed(size_t i) const {
        int k = std::max(1, cfg_.breadth_smooth); double s = 0; int cnt = 0;
        for (int j = 0; j < k && (int)i - j >= 0; ++j) { s += breadth(i - j); ++cnt; }
        return cnt > 0 ? s / cnt : 0.0;
    }
    double breadth_latest() const { return days_.empty() ? 0.0 : breadth(days_.size()-1); }
    double dispersion(size_t i) const {
        std::vector<double> rs;
        for (auto& kv : c_) { if (!eligible(kv.first, i)) continue;
            double r = ret(kv.first, i, cfg_.breadth_lb); if (!std::isnan(r)) rs.push_back(r); }
        if (rs.size() < 2) return 0.0;
        double m = 0; for (double x : rs) m += x; m /= rs.size();
        double var = 0; for (double x : rs) var += (x-m)*(x-m); var /= rs.size();
        return var > 0 ? std::sqrt(var) : 0.0;
    }
    double dispersion_latest() const { return days_.empty() ? 0.0 : dispersion(days_.size()-1); }

    // ===== BATCH SIMULATION (the BACKTEST_TRUTH gate) =======================
    // Returns net daily portfolio returns (turnover-costed). Long-only spot.
    std::vector<std::pair<int64_t,double>> simulate() {
        finalize();
        std::vector<std::pair<int64_t,double>> daily;
        if (days_.size() < 2) return daily;
        std::map<std::string,Position> pos;   // open longs
        std::map<std::string,double> wts;      // yesterday's weights
        int64_t last_step = INT64_MIN/2;
        for (size_t i = 1; i < days_.size(); ++i) {
            // realise the day's return on yesterday's weights
            double r = 0.0;
            for (auto& kv : wts) { double wt = kv.second; if (wt <= 0) continue;
                const auto& v = c_.at(kv.first); double a = v[i-1], b = v[i];
                if (!std::isnan(a) && !std::isnan(b) && a > 0) r += wt * (b/a - 1.0); }
            // advance held bookkeeping
            for (auto& kv : pos) { auto& p = kv.second; ++p.bars_held;
                double cl = c_.at(kv.first)[i];
                if (!std::isnan(cl) && cl > p.highest_close) p.highest_close = cl; }
            if (days_[i] - last_step < cfg_.rebalance_gap) { daily.push_back({days_[i], r}); continue; }
            last_step = days_[i];
            auto nw = portfolio_step(i, pos);   // updates pos (exits + entries), returns weights
            double turn = 0.0; std::set<std::string> allk;
            for (auto& kv : wts) allk.insert(kv.first);
            for (auto& kv : nw)  allk.insert(kv.first);
            for (auto& k : allk) turn += std::fabs((nw.count(k)?nw[k]:0.0) - (wts.count(k)?wts[k]:0.0));
            r -= turn * cfg_.cost_bps/10000.0;
            wts = nw;
            daily.push_back({days_[i], r});
        }
        return daily;
    }

    // Compute the CURRENT target weights at index i (for startup snapshot / live).
    // Does not mutate persistent live state; uses a scratch position map derived
    // from `held`.
    std::map<std::string,double> compute_target_weights(size_t i, bool& bull_out,
                                    const std::map<std::string,Position>& held = {}) {
        finalize();
        std::map<std::string,Position> pos = held;
        auto w = portfolio_step(i, pos, &bull_out);
        return w;
    }

    // ---- live tick path: aggregate to daily OHLCV, step once per new day ----
    void on_tick(const std::string& sym, double price, int64_t now_ms) {
        if (price <= 0 || !o_.count(sym)) return;
        int64_t day = now_ms / 86400000LL;
        if (cur_day_ < 0) cur_day_ = day;
        if (day > cur_day_) {
            for (auto& kv : bar_) { auto& b = kv.second;
                if (b.have) ingest(kv.first, cur_day_, b.o, b.h, b.l, b.c, 0.0); }
            bar_.clear(); dirty_ = true; cur_day_ = day;
            finalize();
            if (!days_.empty()) {
                size_t i = days_.size()-1; bool bull;
                auto w = portfolio_step(i, live_pos_, &bull);
                double turn = 0.0; std::set<std::string> allk;
                for (auto& kv : live_wts_) allk.insert(kv.first);
                for (auto& kv : w) allk.insert(kv.first);
                for (auto& k : allk) turn += std::fabs((w.count(k)?w[k]:0.0)-(live_wts_.count(k)?live_wts_[k]:0.0));
                if (turn > 1e-9) { live_wts_ = w; if (on_rebalance_) on_rebalance_(day, w, bull); }
            }
        }
        auto& b = bar_[sym];
        if (!b.have) { b.o = b.h = b.l = b.c = price; b.have = true; }
        else { b.h = std::max(b.h, price); b.l = std::min(b.l, price); b.c = price; }
    }

protected:
    LODConfig cfg_;

    // ---- family hooks (override) -------------------------------------------
    virtual bool   entry_signal(const std::string& sym, size_t i) const = 0;
    virtual bool   exit_signal (const std::string& sym, size_t i, const Position& p) const = 0;
    virtual double entry_score (const std::string& sym, size_t i) const { return ret(sym, i, 30); }

    // ---- indicator accessors (precomputed by finalize) ---------------------
    double emaF(const std::string& s, size_t i) const { return at(ema_fast_, s, i); }
    double emaS(const std::string& s, size_t i) const { return at(ema_slow_, s, i); }
    double atr (const std::string& s, size_t i) const { return at(atr_, s, i); }
    double rsi (const std::string& s, size_t i) const { return at(rsi_, s, i); }
    double bbMid(const std::string& s, size_t i) const { return at(bb_mid_, s, i); }
    double bbUp (const std::string& s, size_t i) const { return at(bb_up_, s, i); }
    double bbLo (const std::string& s, size_t i) const { return at(bb_lo_, s, i); }
    double bandwidth(const std::string& s, size_t i) const { return at(bandw_, s, i); }
    double volAvg(const std::string& s, size_t i) const { return at(vavg_, s, i); }
    double closeAt(const std::string& s, size_t i) const { return at(c_, s, i); }
    double openAt (const std::string& s, size_t i) const { return at(o_, s, i); }
    double highAt (const std::string& s, size_t i) const { return at(h_, s, i); }
    double lowAt  (const std::string& s, size_t i) const { return at(l_, s, i); }
    double volAt  (const std::string& s, size_t i) const { return at(v_, s, i); }
    double realizedVol(const std::string& s, size_t i, int n) const {
        auto it = c_.find(s); if (it == c_.end()) return NAN; const auto& v = it->second;
        if ((int)i < n+1) return NAN; std::vector<double> rs;
        for (size_t j = i-n; j < i; ++j){ double a=v[j-1],b=v[j]; if(!std::isnan(a)&&!std::isnan(b)&&a>0) rs.push_back(b/a-1.0); }
        if ((int)rs.size() < n*0.6) return NAN;
        double m=0; for(double x:rs)m+=x; m/=rs.size();
        double var=0; for(double x:rs)var+=(x-m)*(x-m); var/=rs.size();
        return var>0?std::sqrt(var):NAN;
    }
    // trailing return over lb
    double ret(const std::string& s, size_t i, int lb) const {
        auto it = c_.find(s); if (it == c_.end()) return NAN; const auto& v = it->second;
        if ((int)i < lb || i >= v.size()) return NAN;
        double a = v[i-lb], b = v[i]; if (std::isnan(a)||std::isnan(b)||a<=0) return NAN;
        return b/a - 1.0;
    }
    // highest close over the last n bars (excludes i)
    double highestClose(const std::string& s, size_t i, int n) const {
        auto it = c_.find(s); if (it==c_.end()||(int)i<n) return NAN; const auto& v=it->second;
        double m=-1e18; for(size_t j=i-n;j<i;++j) if(!std::isnan(v[j])) m=std::max(m,v[j]);
        return m>-1e17?m:NAN;
    }
    // bandwidth percentile over the last lb bars (rank of current among past)
    double bandwidthPct(const std::string& s, size_t i, int lb) const {
        auto it = bandw_.find(s); if (it==bandw_.end()||(int)i<lb) return NAN; const auto& v=it->second;
        double cur=v[i]; if(std::isnan(cur)) return NAN; int le=0,n=0;
        for(size_t j=i-lb;j<i;++j) if(!std::isnan(v[j])){++n; if(v[j]<=cur)++le;}
        return n>0?(double)le/n:NAN;
    }

private:
    std::vector<int64_t> days_;
    std::map<int64_t,size_t> day_idx_;
    std::map<std::string,std::vector<double>> o_,h_,l_,c_,v_;
    // precomputed indicator arrays
    std::map<std::string,std::vector<double>> ema_fast_, ema_slow_, atr_, rsi_,
        bb_mid_, bb_up_, bb_lo_, bandw_, vavg_, dvol_;
    bool dirty_ = true;
    // live
    struct LiveBar { double o=0,h=0,l=0,c=0; bool have=false; };
    std::map<std::string,LiveBar> bar_;
    std::map<std::string,Position> live_pos_;
    std::map<std::string,double> live_wts_;
    int64_t cur_day_ = -1;
    RebalanceCallback on_rebalance_;

    static double at(const std::map<std::string,std::vector<double>>& m,
                     const std::string& s, size_t i) {
        auto it = m.find(s); if (it==m.end()||i>=it->second.size()) return NAN; return it->second[i];
    }
    void ingest(const std::string& sym, int64_t day,
                double o, double hi, double lo, double cl, double vol) {
        if (!o_.count(sym)) { o_[sym]={};h_[sym]={};l_[sym]={};c_[sym]={};v_[sym]={}; }
        if (!day_idx_.count(day)) { day_idx_[day]=days_.size(); days_.push_back(day);
            for (auto& kv : o_) kv.second.resize(days_.size(),NAN);
            for (auto& kv : h_) kv.second.resize(days_.size(),NAN);
            for (auto& kv : l_) kv.second.resize(days_.size(),NAN);
            for (auto& kv : c_) kv.second.resize(days_.size(),NAN);
            for (auto& kv : v_) kv.second.resize(days_.size(),NAN); }
        size_t i = day_idx_[day];
        for (auto* mp : {&o_,&h_,&l_,&c_,&v_}) if ((*mp)[sym].size()<days_.size()) (*mp)[sym].resize(days_.size(),NAN);
        o_[sym][i]=o; h_[sym][i]=hi; l_[sym][i]=lo; c_[sym][i]=cl; v_[sym][i]=vol;
    }
    double avg_dollar_vol(const std::string& sym, size_t i, int n) const {
        auto it = dvol_.find(sym); if (it==dvol_.end()) return NAN; const auto& d=it->second;
        if ((int)i<n) return NAN; double s=0; int cnt=0;
        for(size_t j=i-n;j<i;++j) if(!std::isnan(d[j])&&d[j]>0){s+=d[j];++cnt;}
        return cnt>0?s/cnt:NAN;
    }

    // precompute the standard indicator set once (cleared on new data)
    void finalize() {
        if (!dirty_) return; dirty_ = false;
        size_t n = days_.size();
        for (auto& kv : c_) {
            const std::string& s = kv.first; const auto& C = kv.second;
            const auto& H = h_.at(s); const auto& L = l_.at(s); const auto& V = v_.at(s);
            auto& ef = ema_fast_[s]; auto& es = ema_slow_[s]; auto& a = atr_[s]; auto& rs = rsi_[s];
            auto& bm = bb_mid_[s]; auto& bu = bb_up_[s]; auto& bl = bb_lo_[s]; auto& bw = bandw_[s];
            auto& va = vavg_[s]; auto& dv = dvol_[s];
            ef.assign(n,NAN); es.assign(n,NAN); a.assign(n,NAN); rs.assign(n,NAN);
            bm.assign(n,NAN); bu.assign(n,NAN); bl.assign(n,NAN); bw.assign(n,NAN);
            va.assign(n,NAN); dv.assign(n,NAN);
            double kf = 2.0/(cfg_.ema_fast_n+1), ks = 2.0/(cfg_.ema_slow_n+1);
            double emaf=NAN, emas=NAN, atrv=NAN; int seen=0;
            double gain=0, loss=0; // Wilder RSI
            for (size_t i=0;i<n;++i) {
                double cl=C[i]; if (std::isnan(cl)||cl<=0) continue;
                dv[i] = (!std::isnan(V[i])&&V[i]>0)? cl*V[i] : NAN;
                // EMA
                emaf = std::isnan(emaf)? cl : cl*kf + emaf*(1-kf);
                emas = std::isnan(emas)? cl : cl*ks + emas*(1-ks);
                ef[i]=emaf; es[i]=emas;
                // ATR (Wilder) using true range
                if (i>0 && !std::isnan(C[i-1])) {
                    double tr = std::max({H[i]-L[i], std::fabs(H[i]-C[i-1]), std::fabs(L[i]-C[i-1])});
                    atrv = std::isnan(atrv)? tr : (atrv*(cfg_.atr_n-1)+tr)/cfg_.atr_n;
                    a[i]=atrv;
                }
                // RSI (Wilder)
                if (i>0 && !std::isnan(C[i-1])) {
                    double ch = cl - C[i-1]; double g = ch>0?ch:0, ls = ch<0?-ch:0;
                    if (seen < cfg_.rsi_n) { gain+=g; loss+=ls; ++seen;
                        if (seen==cfg_.rsi_n){ gain/=cfg_.rsi_n; loss/=cfg_.rsi_n;
                            double rs0 = loss>0?gain/loss:999; rs[i]=100-100/(1+rs0); } }
                    else { gain=(gain*(cfg_.rsi_n-1)+g)/cfg_.rsi_n; loss=(loss*(cfg_.rsi_n-1)+ls)/cfg_.rsi_n;
                        double rs0 = loss>0?gain/loss:999; rs[i]=100-100/(1+rs0); }
                }
                // Bollinger (SMA + stdev over bb_n) + bandwidth
                if ((int)i >= cfg_.bb_n-1) {
                    double sum=0,sq=0; int cnt=0; bool ok=true;
                    for (int j=(int)i-cfg_.bb_n+1;j<=(int)i;++j){ double x=C[j];
                        if(std::isnan(x)){ok=false;break;} sum+=x; sq+=x*x; ++cnt; }
                    if (ok && cnt==cfg_.bb_n){ double mean=sum/cnt; double var=sq/cnt-mean*mean;
                        double sd=var>0?std::sqrt(var):0; bm[i]=mean; bu[i]=mean+cfg_.bb_k*sd; bl[i]=mean-cfg_.bb_k*sd;
                        bw[i]= mean>0? (bu[i]-bl[i])/mean : NAN; }
                }
                // avg base volume
                if ((int)i >= cfg_.vol_avg_n) { double sum=0;int cnt=0; bool ok=true;
                    for(int j=(int)i-cfg_.vol_avg_n+1;j<=(int)i;++j){ if(std::isnan(V[j])){ok=false;break;} sum+=V[j];++cnt; }
                    if(ok&&cnt>0) va[i]=sum/cnt; }
            }
        }
    }

    // one portfolio step at index i: apply exits, then breadth-gated entries,
    // size (inverse-vol or equal, per-name capped), return target weights.
    std::map<std::string,double> portfolio_step(size_t i, std::map<std::string,Position>& pos,
                                                bool* bull_out = nullptr) {
        // 1. exits
        std::vector<std::string> to_close;
        for (auto& kv : pos) if (exit_signal(kv.first, i, kv.second)) to_close.push_back(kv.first);
        for (auto& s : to_close) pos.erase(s);
        // 2. regime gate for NEW entries (NO 200DMA)
        bool bull = true;
        if (cfg_.breadth_gate) bull = breadth_smoothed(i) >= cfg_.breadth_thresh;
        if (bull_out) *bull_out = bull;
        // 3. entries (only when bull, up to max_positions)
        if (bull && (int)pos.size() < cfg_.max_positions) {
            std::vector<std::pair<double,std::string>> cands;
            for (auto& kv : c_) { const std::string& s = kv.first;
                if (pos.count(s)) continue;
                if (!eligible(s, i)) continue;
                if (!entry_signal(s, i)) continue;
                cands.push_back({entry_score(s, i), s}); }
            std::sort(cands.begin(), cands.end(), [](auto&a,auto&b){ return a.first>b.first; });
            for (auto& c : cands) { if ((int)pos.size() >= cfg_.max_positions) break;
                Position p; p.entry_price = c_.at(c.second)[i];
                p.entry_atr = atr(c.second, i); p.entry_vol = realizedVol(c.second, i, cfg_.vol_window);
                p.highest_close = p.entry_price; p.bars_held = 0; p.entry_i = i;
                pos[c.second] = p; }
        }
        // 4. size
        std::map<std::string,double> w;
        if (pos.empty()) return w;
        std::vector<std::string> names; for (auto& kv : pos) names.push_back(kv.first);
        if (cfg_.inverse_vol) {
            std::map<std::string,double> iv; double tot=0;
            for (auto& s : names){ double vv=realizedVol(s,i,cfg_.vol_window);
                double x=(!std::isnan(vv)&&vv>0)?1.0/vv:0.0; iv[s]=x; tot+=x; }
            if (tot>0) for (auto& s : names) w[s]=iv[s]/tot; else for(auto&s:names) w[s]=1.0/names.size();
        } else for (auto& s : names) w[s]=1.0/names.size();
        // per-name cap + renormalise the capped remainder is left as cash (do not
        // re-lever onto fewer names — keeps risk bounded).
        for (auto& kv : w) kv.second = std::min(kv.second, cfg_.per_name_cap);
        return w;
    }
};

} // namespace chimera

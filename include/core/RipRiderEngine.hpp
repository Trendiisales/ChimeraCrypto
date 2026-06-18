// ============================================================================
//  RipRiderEngine.hpp — per-symbol regime-gated momentum-continuation ("rip rider")
//  (2026-06-18). Catches an individual coin's RIP and RIDES it — long-only spot.
//
//  The validated way to trade rips without killing the fat tail (proven on the
//  53-alt daily universe, cross-regime, backtest/crypto_momo_rider.py):
//    - ENTER: ignition (close up >= ig_pct over lb days) AND BTC>200d (bull regime).
//    - RIDE:  NO tight trail (tight trails amputate — tight-8% kept only +2109% of
//             2021's +18701%). Pure ride + an optional WIDE backstop only.
//    - EXIT:  when the regime flips bear (BTC<200d) — protection comes from the
//             GATE + regime-exit, NOT a per-trade trail.
//  Result (gated ride + regime-exit): 2021 +18701% / 2022-bear FLAT / 2023 +2038 /
//  2024 +214 / 2025 +1747 — keeps the tail, sits out the bear. Sibling/complement to
//  CrossSectionalMomentumEngine (portfolio rotation); this is per-symbol continuation.
//
//  Self-aggregates daily closes from on_tick (like CrossSectionalMomentumEngine).
//  SHADOW by default. Warm-seed >=200 daily closes (BTC SMA) before live.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <functional>
#include <cstdint>

namespace chimera {

// Self-contained close record (Chimera has no omega::TradeRecord).
struct RipClose {
    std::string symbol, side, engine, exitReason;
    double entryPrice=0, exitPrice=0, size=0, pnl=0;
    int64_t exitTs=0;
};

struct RipRiderConfig {
    double ig_pct        = 0.20;   // ignition: close up >= this over lb days = "a rip"
    int    lb_days       = 5;
    int    btc_sma_days  = 200;    // regime gate
    bool   regime_gate   = true;   // enter only when BTC>200d (LOAD-BEARING)
    bool   regime_exit   = true;   // exit when BTC<200d (the protection — NOT a tight trail)
    double wide_trail    = 0.0;    // 0 = pure ride; else a WIDE backstop (e.g. 0.50) for single-name blowups
    int    maxhold_days  = 60;     // hard backstop (matches validated backtest MAXHOLD=60)
    std::string gate_symbol = "BTC";
    double dollars_per_pt = 1.0;
    double lot           = 1.0;
};

class RipRiderEngine {
public:
    bool shadow_mode = true, enabled = true;
    std::string engine_name = "RipRider";
    using CloseCallback = std::function<void(const RipClose&)>;
    using EntryCallback = std::function<void(const std::string& sym, double entry_px, int64_t ts)>;

    explicit RipRiderEngine(RipRiderConfig cfg = {}) : cfg_(cfg) {}
    void set_universe(const std::vector<std::string>& syms){ for(auto&s:syms) if(!sd_.count(s)){ sd_[s]={}; so_[s]={}; pos_[s]={}; } }
    void set_close_callback(CloseCallback cb){ on_close_ = std::move(cb); }
    void set_entry_callback(EntryCallback cb){ on_entry_ = std::move(cb); }

    void seed_daily_bar(const std::string& sym, int64_t day, double open, double close){ ingest(sym, day, open, close); }
    void seed_daily_close(const std::string& sym, int64_t day, double close){ ingest(sym, day, close, close); }
    size_t num_days() const { return days_.size(); }

    // live tick path: aggregate to daily close; on UTC day rollover, run per-symbol logic
    void on_tick(const std::string& sym, double price, int64_t now_ms){
        if(price<=0 || !sd_.count(sym)) return;
        int64_t day = now_ms/86400000LL;
        if(cur_day_<0) cur_day_=day;
        if(day>cur_day_){
            for(auto&kv:last_px_) ingest(kv.first, cur_day_, day_open_.count(kv.first)?day_open_[kv.first]:kv.second, kv.second);
            cur_day_=day; day_open_.clear();
            evaluate_day(days_.size()? days_.size()-1 : 0, now_ms);  // run ignition/manage on the just-closed day
        }
        if(!day_open_.count(sym)) day_open_[sym]=price;  // first tick of the day = open
        last_px_[sym]=price;
    }

    // ===== PURE LOGIC (faithful to crypto_momo_rider.py gated ride+regime-exit) =====
    bool btc_bull(size_t i) const {
        auto it=sd_.find(cfg_.gate_symbol); if(it==sd_.end()) return true;
        double m=sma(it->second,i,cfg_.btc_sma_days);
        double c=(i<it->second.size())?it->second[i]:NAN;
        if(std::isnan(m)||std::isnan(c)) return true;  // pre-history: allow
        return c>m;
    }

    // evaluate ignition + position management at dense-axis index i; emit closes via cb
    void evaluate_day(size_t i, int64_t now_ms){
        if(i==0) return;
        for(auto& kv : sd_){
            const std::string& s = kv.first;
            if(s==cfg_.gate_symbol) continue;
            const auto& px = kv.second;
            if(i>=px.size()) continue;
            Pos& p = pos_[s];
            double c = px[i];
            if(p.active){
                if(!std::isnan(c) && c>p.peak) p.peak=c;
                bool exit=false; const char* why="";
                if(cfg_.regime_exit && cfg_.regime_gate && !btc_bull(i)){ exit=true; why="REGIME"; }
                else if(cfg_.wide_trail>0 && !std::isnan(c) && c <= p.peak*(1-cfg_.wide_trail)){ exit=true; why="WIDE_TRAIL"; }
                else if((int)(days_[i]-p.entry_day) >= cfg_.maxhold_days){ exit=true; why="MAXHOLD"; }
                if(exit && !std::isnan(c)) close_pos(s, c, why, now_ms);
            } else {
                // ignition on day i's close -> ENTER at NEXT day's OPEN (faithful to
                // crypto_momo_rider.py: entry=bars[i+1].open), gate checked at entry day.
                double tr = trailing_ret(px, i, cfg_.lb_days);
                if(!std::isnan(tr) && tr >= cfg_.ig_pct && i+1 < days_.size()){
                    bool gate_ok = !cfg_.regime_gate || btc_bull(i+1);
                    auto oit = so_.find(s);
                    double e = (oit!=so_.end() && i+1 < oit->second.size()) ? oit->second[i+1] : NAN;
                    if(gate_ok && !std::isnan(e) && e>0){
                        p.active=true; p.entry=e; p.peak=e; p.entry_day=days_[i+1]; ++trade_id_;
                        if(on_entry_) on_entry_(s, e, now_ms);
                    }
                }
            }
        }
    }

    bool has_open_position() const { for(auto&kv:pos_) if(kv.second.active) return true; return false; }

    // full historical replay for the faithful BT (emits closes via the callback)
    void simulate(){ for(size_t i=1;i<days_.size();++i) evaluate_day(i, days_[i]*86400000LL); }

private:
    RipRiderConfig cfg_;
    std::vector<int64_t> days_;
    std::map<int64_t,size_t> day_idx_;
    std::map<std::string,std::vector<double>> sd_;   // sym -> daily closes (nan-padded dense)
    std::map<std::string,std::vector<double>> so_;   // sym -> daily opens  (nan-padded dense)
    std::map<std::string,double> last_px_, day_open_;
    struct Pos{ bool active=false; double entry=0,peak=0; int64_t entry_day=0; };
    std::map<std::string,Pos> pos_;
    int64_t cur_day_=-1; int trade_id_=0;
    CloseCallback on_close_;
    EntryCallback on_entry_;

    void close_pos(const std::string& s, double exit_px, const char* why, int64_t now_ms){
        Pos& p=pos_[s];
        RipClose tr{};
        tr.symbol=s; tr.side="LONG"; tr.engine=engine_name;
        tr.entryPrice=p.entry; tr.exitPrice=exit_px; tr.size=cfg_.lot;
        tr.pnl=(exit_px-p.entry)*cfg_.lot*cfg_.dollars_per_pt; tr.exitReason=why; tr.exitTs=now_ms/1000;
        if(on_close_) on_close_(tr);
        p=Pos{};
    }
    void ingest(const std::string& sym, int64_t day, double open, double close){
        if(!sd_.count(sym)){ sd_[sym]={}; so_[sym]={}; pos_[sym]={}; }
        if(!day_idx_.count(day)){ day_idx_[day]=days_.size(); days_.push_back(day);
            for(auto&kv:sd_) kv.second.resize(days_.size(),NAN);
            for(auto&kv:so_) kv.second.resize(days_.size(),NAN); }
        size_t i=day_idx_[day];
        if(sd_[sym].size()<days_.size()) sd_[sym].resize(days_.size(),NAN);
        if(so_[sym].size()<days_.size()) so_[sym].resize(days_.size(),NAN);
        sd_[sym][i]=close; so_[sym][i]=open;
    }
    static double sma(const std::vector<double>& s, size_t i, int n){
        if((int)i<n) return NAN; double sum=0; int c=0;
        for(size_t j=i-n;j<i;++j) if(!std::isnan(s[j])){sum+=s[j];++c;}
        return c>=n*0.8 ? sum/c : NAN;
    }
    static double trailing_ret(const std::vector<double>& s, size_t i, int lb){
        if((int)i<lb||i>=s.size()) return NAN;
        double a=s[i-lb],b=s[i]; if(std::isnan(a)||std::isnan(b)||a<=0) return NAN;
        return b/a-1.0;
    }
};

} // namespace chimera

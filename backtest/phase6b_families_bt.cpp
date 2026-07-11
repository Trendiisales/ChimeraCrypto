// ============================================================================
// phase6b_families_bt.cpp — Phase-6b REMAINING long-only families, SALVAGE CHECK.
//
//   Families:  1 breakout-retest      2 relative-strength acceleration
//              3 BTC-lead alt confirm  4 breadth-thrust
//              5 capitulation-recovery 6 liquidity-sweep-reversal
//              7 young-liquid-coin momentum
//
// CONTROL-FIRST. Phase 6's decisive finding: the 3 priority families FAILED an
// exposure-matched random pick-edge control (a breadth-gated RANDOM basket ~ ex-
// 2022 Sharpe 1.20) — their edge was breadth TIMING, not entry SELECTION. So the
// SAME validate() harness leads with that control: a family is only a build/wire
// candidate if its ex-2022 Sharpe BEATS the random draw MEDIAN *and* it improves
// the combined book (low corr to XSec v1). Expect most to fail; report all.
//
// Long-only spot, NO shorts, NO 200DMA (regime = smoothed BREADTH). 2022 SHOWN,
// NOT gated (feedback-crypto-omit-2022-longonly). Identical seed + costs as P6.
//
//   build:  g++ -std=c++20 -O2 -I../include phase6b_families_bt.cpp -o phase6b_families_bt
//   run:    ./phase6b_families_bt      (from backtest/, needs ../data/xsec_seed)
// ============================================================================
#include "core/LongOnlyDailyBase.hpp"
#include "core/BreakoutRetestEngine.hpp"
#include "core/RelStrengthAccelEngine.hpp"
#include "core/BtcLeadAltEngine.hpp"
#include "core/BreadthThrustEngine.hpp"
#include "core/CapitulationRecoveryEngine.hpp"
#include "core/LiqSweepReversalEngine.hpp"
#include "core/YoungCoinMomoEngine.hpp"
#include "core/CrossSectionalMomentumEngine.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <functional>
#include <algorithm>

using namespace chimera;

struct Row { int64_t day; double o,h,l,c,v; };
static std::map<std::string,std::vector<Row>> load_universe(const std::vector<std::string>& syms,
                                                            const std::string& dir) {
    std::map<std::string,std::vector<Row>> out;
    for (auto& s : syms) {
        std::string path = dir + "/" + s + "USDT_1d.csv";
        FILE* f = fopen(path.c_str(), "r"); if (!f) continue;
        char line[512]; bool first = true;
        while (fgets(line, sizeof line, f)) {
            if (first) { first = false; if (!isdigit((unsigned char)line[0])) continue; }
            char* p = line; char* e;
            long long ts = strtoll(p, &e, 10); if (e==p) continue;
            double o=strtod(e+1,&e), h=strtod(e+1,&e), l=strtod(e+1,&e), c=strtod(e+1,&e), v=strtod(e+1,&e);
            if (c>0) out[s].push_back({ts/86400000LL,o,h,l,c,v});
        }
        fclose(f);
    }
    return out;
}

struct Metrics { double total=0,sum=0,sharpe=0,maxdd=0,pf=0; int days=0,active=0; };
static Metrics metrics(const std::vector<std::pair<int64_t,double>>& d) {
    Metrics m; m.days=(int)d.size(); if (d.empty()) return m;
    double eq=1,peak=1,mean=0,gp=0,gl=0;
    for (auto& kv:d){ double r=kv.second; m.sum+=r; mean+=r; if(std::fabs(r)>1e-12)m.active++;
        if(r>0)gp+=r; else gl+=-r; eq*=(1+r); peak=std::max(peak,eq); m.maxdd=std::max(m.maxdd,(peak-eq)/peak); }
    m.total=eq-1; mean/=d.size();
    double var=0; for(auto&kv:d){double x=kv.second-mean;var+=x*x;} var/=d.size();
    double sd=var>0?std::sqrt(var):0; m.sharpe=sd>0?mean/sd*std::sqrt(365.0):0;
    m.pf=gl>0?gp/gl:(gp>0?999:0); return m;
}
static std::vector<std::pair<int64_t,double>> half(const std::vector<std::pair<int64_t,double>>& d,int which){
    std::vector<std::pair<int64_t,double>> o; size_t n=d.size();
    for(size_t i=0;i<n;++i) if((which==0)==(i<n/2)) o.push_back(d[i]); return o; }
static std::vector<std::pair<int64_t,double>> window(const std::vector<std::pair<int64_t,double>>& d,int64_t lo,int64_t hi){
    std::vector<std::pair<int64_t,double>> o; for(auto&kv:d){int64_t t=kv.first*86400000LL; if(t>=lo&&t<hi)o.push_back(kv);} return o; }
static std::vector<std::pair<int64_t,double>> drop(const std::vector<std::pair<int64_t,double>>& d,int64_t lo,int64_t hi){
    std::vector<std::pair<int64_t,double>> o; for(auto&kv:d){int64_t t=kv.first*86400000LL; if(!(t>=lo&&t<hi))o.push_back(kv);} return o; }
static std::vector<std::pair<int64_t,double>> blend(const std::vector<std::pair<int64_t,double>>& a,
        const std::vector<std::pair<int64_t,double>>& b,double wa,double wb){
    std::map<int64_t,double> m; for(auto&kv:a)m[kv.first]+=wa*kv.second; for(auto&kv:b)m[kv.first]+=wb*kv.second;
    return {m.begin(),m.end()}; }
static double corr_of(const std::vector<std::pair<int64_t,double>>& A,const std::vector<std::pair<int64_t,double>>& B){
    std::map<int64_t,double> ma,mb; for(auto&kv:A)ma[kv.first]=kv.second; for(auto&kv:B)mb[kv.first]=kv.second;
    std::vector<double> a,b; for(auto&kv:ma){auto it=mb.find(kv.first); if(it!=mb.end()){a.push_back(kv.second);b.push_back(it->second);}}
    if(a.size()<10) return 0; double m1=0,m2=0; for(size_t i=0;i<a.size();++i){m1+=a[i];m2+=b[i];} m1/=a.size();m2/=b.size();
    double sab=0,saa=0,sbb=0; for(size_t i=0;i<a.size();++i){double x=a[i]-m1,y=b[i]-m2;sab+=x*y;saa+=x*x;sbb+=y*y;}
    return (saa>0&&sbb>0)?sab/std::sqrt(saa*sbb):0; }

static const std::vector<std::string> UNIVERSE = {
    "AAVE","ADA","ALGO","APE","AR","ATOM","AVAX","AXS","BAT","BCH","BNB","BTC",
    "CAKE","CHZ","COMP","CRV","DASH","DOGE","DOT","EGLD","ENJ","ETC","ETH","FET",
    "FIL","GALA","GRT","HBAR","ICP","INJ","KSM","LDO","LINK","LTC","MANA","NEAR",
    "NEO","ONE","QTUM","RUNE","SAND","SHIB","SNX","SOL","SUSHI","THETA","TRX",
    "UNI","VET","XLM","XRP","XTZ","ZEC","ZIL" };

static const int64_t X22lo=1640995200000LL, X22hi=1672531200000LL; // 2022 bear
static const int64_t X25lo=1735689600000LL, X25hi=1767225600000LL; // 2025 (alt holdout)

static std::map<std::string,std::vector<Row>>* g_data = nullptr;

template<class Eng, class Cfg>
static std::vector<std::pair<int64_t,double>> run_family(const Cfg& cfg) {
    Eng e(cfg); e.set_universe(UNIVERSE);
    for (auto& kv : *g_data) for (auto& r : kv.second) e.seed_daily(kv.first, r.day, r.o, r.h, r.l, r.c, r.v);
    return e.simulate();
}

static std::vector<std::pair<int64_t,double>> build_xsec_v1() {
    XSecConfig b; b.gate_mode=0;  XSecConfig r; r.gate_mode=1; r.breadth_thresh=0.65;
    auto mk=[&](const XSecConfig& c){ auto* e=new CrossSectionalMomentumEngine(c); e->set_universe(UNIVERSE);
        for(auto&kv:*g_data) for(auto&rw:kv.second) e->seed_daily_close(kv.first,rw.day,rw.c); return e; };
    auto* eb=mk(b); auto* er=mk(r); auto out=blend(eb->simulate(),er->simulate(),0.6,0.4);
    delete eb; delete er; return out;
}

static void pr(const char* tag, const Metrics& m){
    printf("  %-16s total=%+10.1f%%  Sharpe=%5.2f  maxDD=%5.1f%%  PF=%4.2f  days=%d active=%d\n",
           tag, m.total*100, m.sharpe, m.maxdd*100, m.pf, m.days, m.active); }

// VERDICT accumulator (printed as a table at the end)
struct Verdict { std::string name; double ex_sharpe=0, ctrl_med=0; double ctrl_beat_pct=0;
                 double corr=0, comb_sh=0, base_sh=0, comb_dd=0, base_dd=0; int active=0;
                 bool beats_ctrl=false, improves=false; };
static std::vector<Verdict> g_verdicts;

template<class Eng, class Cfg>
static void validate(const char* name, const Cfg& base,
                     const std::vector<std::pair<int64_t,double>>& xsec_v1,
                     std::function<Cfg(Cfg,double)> cost_mut,
                     std::vector<std::pair<const char*,std::function<Cfg(Cfg)>>> plateau) {
    Verdict V; V.name = name;
    printf("\n############################################################\n");
    printf("### FAMILY: %s\n", name);
    printf("############################################################\n");
    auto daily = run_family<Eng,Cfg>(base);
    Metrics m = metrics(daily);
    printf("\n--- HEADLINE (net 15bp/side, full history) ---\n"); pr(name, m);

    auto ex = drop(daily, X22lo, X22hi); Metrics me = metrics(ex);
    printf("\n--- EX-2022 (bear NOT gated) ---\n"); pr("ex-2022", me);
    V.ex_sharpe = me.sharpe; V.active = me.active;
    Metrics ea=metrics(half(ex,0)), eb=metrics(half(ex,1));
    printf("  ex22 WF: H1 %+8.1f%% Sh%5.2f | H2 %+8.1f%% Sh%5.2f  %s\n",
           ea.total*100,ea.sharpe,eb.total*100,eb.sharpe,(ea.total>0&&eb.total>0)?"BOTH+":"FAIL");

    Metrics ha=metrics(half(daily,0)), hb=metrics(half(daily,1));
    printf("\n--- WALK-FORWARD both halves (full) ---\n");
    printf("  H1 %+8.1f%% Sh%5.2f | H2 %+8.1f%% Sh%5.2f  %s\n",
           ha.total*100,ha.sharpe,hb.total*100,hb.sharpe,(ha.total>0&&hb.total>0)?"BOTH+":"FAIL");

    Metrics b22=metrics(window(daily,X22lo,X22hi)), b25=metrics(window(daily,X25lo,X25hi));
    printf("\n--- BEAR behaviour (shown, not gated) ---\n");
    printf("  2022 total=%+7.1f%% (active %d/%d)  |  2025 total=%+7.1f%% (active %d/%d)\n",
           b22.total*100,b22.active,b22.days,b25.total*100,b25.active,b25.days);

    { Cfg x = cost_mut(base, 30.0); Metrics m2=metrics(run_family<Eng,Cfg>(x));
      auto e2=drop(run_family<Eng,Cfg>(x),X22lo,X22hi); Metrics m2e=metrics(e2);
      printf("\n--- 2x-COST (30bp/side) ---\n");
      printf("  full total=%+8.1f%% Sh%5.2f %s | ex-2022 total=%+8.1f%% %s\n",
             m2.total*100,m2.sharpe,m2.total>0?"+":"FAIL", m2e.total*100, m2e.total>0?"+":"FAIL"); }

    printf("\n--- PARAM-NEIGHBOURHOOD PLATEAU (ex-2022) ---\n");
    for (auto& pv : plateau) { Cfg x = pv.second(base);
        auto e=drop(run_family<Eng,Cfg>(x),X22lo,X22hi); Metrics mm=metrics(e);
        printf("  %-20s ex22 total=%+9.1f%%  Sharpe=%5.2f  %s\n",
               pv.first, mm.total*100, mm.sharpe, mm.total>0?"+":"NEG"); }

    { auto d=ex; int W=30; double best=-1e18; int bi=0;
      for(int i=0;i+W<=(int)d.size();++i){double p=1;for(int j=i;j<i+W;++j)p*=(1+d[j].second); if(p>best){best=p;bi=i;}}
      auto d2=d; for(int j=bi;j<bi+W&&j<(int)d2.size();++j)d2[j].second=0; Metrics mm=metrics(d2);
      printf("\n--- EX-BEST-EPISODE (ex-2022) ---\n");
      printf("  best 30d block x%.2f; ex-episode total=%+8.1f%% Sharpe%5.2f %s\n",
             best,mm.total*100,mm.sharpe,mm.total>0?"STILL+":"FAILS"); }

    // ==== THE SCREEN: exposure-matched pick-edge control (ex-2022 Sharpe) ====
    printf("\n--- RANDOM-ENTRY CONTROL (exposure-matched, ex-2022 Sharpe) [THE SCREEN] ---\n");
    {
        std::mt19937 rng(777);
        std::vector<int64_t> days; { std::set<int64_t> sd; for(auto&kv:*g_data)for(auto&r:kv.second)sd.insert(r.day);
            days.assign(sd.begin(),sd.end()); std::sort(days.begin(),days.end()); }
        std::map<std::string,std::map<int64_t,double>> px;
        for(auto&kv:*g_data)for(auto&r:kv.second)px[kv.first][r.day]=r.c;
        auto elig_at=[&](int64_t d){ std::vector<std::string> el;
            for(auto&kv:px){auto b=kv.second.find(d),h=kv.second.find(d-120);
                if(b!=kv.second.end()&&h!=kv.second.end())el.push_back(kv.first);} return el; };
        auto breadth_at=[&](int64_t d){ auto el=elig_at(d); if(el.empty())return 0.0; int p=0;
            for(auto&s:el){auto b=px[s].find(d),a=px[s].find(d-30);
                if(b!=px[s].end()&&a!=px[s].end()&&a->second>0&&b->second/a->second-1>0)++p;}
            return (double)p/el.size(); };
        int N=200; int K=base.max_positions>0?std::min(base.max_positions,4):4;
        std::vector<double> sharpes;
        for(int it=0;it<N;++it){ std::map<std::string,double> w; int64_t last=-1<<30;
            std::vector<std::pair<int64_t,double>> rr;
            for(size_t i=1;i<days.size();++i){ double ret=0;
                for(auto&kv:w){auto&mp=px[kv.first]; auto a=mp.find(days[i-1]),b=mp.find(days[i]);
                    if(a!=mp.end()&&b!=mp.end()&&a->second>0) ret+=kv.second*(b->second/a->second-1);}
                if(days[i]-last>=14){ last=days[i]; std::map<std::string,double> nw;
                    if(breadth_at(days[i])>=base.breadth_thresh){ auto el=elig_at(days[i]);
                        std::shuffle(el.begin(),el.end(),rng);
                        for(int k=0;k<K&&k<(int)el.size();++k)nw[el[k]]=1.0/K; }
                    double turn=0; std::set<std::string> all; for(auto&kv:w)all.insert(kv.first); for(auto&kv:nw)all.insert(kv.first);
                    for(auto&k:all)turn+=std::fabs((nw.count(k)?nw[k]:0)-(w.count(k)?w[k]:0));
                    ret-=turn*15.0/10000.0; w=nw; }
                rr.push_back({days[i],ret}); }
            sharpes.push_back(metrics(drop(rr,X22lo,X22hi)).sharpe); }
        std::sort(sharpes.begin(),sharpes.end());
        double med=sharpes[sharpes.size()/2], p90=sharpes[(size_t)(sharpes.size()*0.90)];
        int beat=0; for(double s:sharpes) if(me.sharpe>s)++beat; double pct=100.0*beat/std::max(1,(int)sharpes.size());
        V.ctrl_med=med; V.ctrl_beat_pct=pct; V.beats_ctrl=(me.sharpe>med);
        printf("  random gated pick (N=%d): median Sharpe=%.2f  p90=%.2f\n",N,med,p90);
        printf("  family ex-2022 Sharpe=%.2f -> beats %.0f%% of random draws  %s\n",
               me.sharpe, pct, V.beats_ctrl?"PASS(>median)":"WEAK(<median)");
    }

    printf("\n--- PORTFOLIO-INCREMENT (family alongside XSec v1) ---\n");
    {
        auto fam = ex; auto base_book = drop(xsec_v1, X22lo, X22hi);
        double rho = corr_of(base_book, fam);
        Metrics mb = metrics(base_book), mf = metrics(fam);
        auto comb = blend(base_book, fam, 0.80, 0.20); Metrics mc = metrics(comb);
        V.corr=rho; V.comb_sh=mc.sharpe; V.base_sh=mb.sharpe; V.comb_dd=mc.maxdd; V.base_dd=mb.maxdd;
        printf("  corr(XSec v1, %s) [ex-2022] = %+.3f  (lower = more diversifying)\n", name, rho);
        pr("XSec v1 alone", mb); pr(name, mf); pr("80/20 combined", mc);
        bool improves = (mc.sharpe > mb.sharpe) || (mc.maxdd < mb.maxdd && mc.sharpe >= mb.sharpe*0.98);
        V.improves = improves;
        printf("  -> combined Sharpe %.2f vs XSec-v1 %.2f, maxDD %.1f%% vs %.1f%% : %s the combined book\n",
               mc.sharpe, mb.sharpe, mc.maxdd*100, mb.maxdd*100, improves?"IMPROVES":"does NOT improve");
    }
    g_verdicts.push_back(V);
    printf("\n");
}

int main(int argc, char** argv) {
    std::string dir = (argc>1)?argv[1]:"../data/xsec_seed";
    auto data = load_universe(UNIVERSE, dir); g_data = &data;
    printf("Loaded %zu/%zu symbols from %s\n", data.size(), UNIVERSE.size(), dir.c_str());

    auto xsec_v1 = build_xsec_v1();
    printf("Reference existing book: XSec v1 dual-sleeve (60/40). "); pr("XSec v1", metrics(xsec_v1));

    validate<BreakoutRetestEngine,BreakoutRetestConfig>(
        "BREAKOUT-RETEST", BreakoutRetestConfig{}, xsec_v1,
        [](BreakoutRetestConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"range_lb=15",[](BreakoutRetestConfig c){c.range_lb=15;return c;}},
          {"range_lb=25",[](BreakoutRetestConfig c){c.range_lb=25;return c;}},
          {"band=0.02",  [](BreakoutRetestConfig c){c.band=0.02;return c;}},
          {"band=0.05",  [](BreakoutRetestConfig c){c.band=0.05;return c;}},
          {"retest_lb=4",[](BreakoutRetestConfig c){c.retest_lb=4;return c;}},
          {"retest_lb=8",[](BreakoutRetestConfig c){c.retest_lb=8;return c;}},
          {"atr=2.5",    [](BreakoutRetestConfig c){c.atr_mult=2.5;return c;}},
          {"atr=3.5",    [](BreakoutRetestConfig c){c.atr_mult=3.5;return c;}},
          {"breadth=0.35",[](BreakoutRetestConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](BreakoutRetestConfig c){c.breadth_thresh=0.45;return c;}} });

    validate<RelStrengthAccelEngine,RelStrengthAccelConfig>(
        "RS-ACCELERATION", RelStrengthAccelConfig{}, xsec_v1,
        [](RelStrengthAccelConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"mom_lb=20",  [](RelStrengthAccelConfig c){c.mom_lb=20;return c;}},
          {"mom_lb=45",  [](RelStrengthAccelConfig c){c.mom_lb=45;return c;}},
          {"acc_short=7",[](RelStrengthAccelConfig c){c.acc_short=7;return c;}},
          {"acc_short=14",[](RelStrengthAccelConfig c){c.acc_short=14;return c;}},
          {"atr=2.5",    [](RelStrengthAccelConfig c){c.atr_mult=2.5;return c;}},
          {"atr=3.5",    [](RelStrengthAccelConfig c){c.atr_mult=3.5;return c;}},
          {"ema 15/40",  [](RelStrengthAccelConfig c){c.ema_fast=15;c.ema_slow=40;return c;}},
          {"ema 25/60",  [](RelStrengthAccelConfig c){c.ema_fast=25;c.ema_slow=60;return c;}},
          {"breadth=0.35",[](RelStrengthAccelConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](RelStrengthAccelConfig c){c.breadth_thresh=0.45;return c;}} });

    validate<BtcLeadAltEngine,BtcLeadAltConfig>(
        "BTC-LEAD-ALT", BtcLeadAltConfig{}, xsec_v1,
        [](BtcLeadAltConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"btc_lb=14",  [](BtcLeadAltConfig c){c.btc_lb=14;return c;}},
          {"btc_lb=30",  [](BtcLeadAltConfig c){c.btc_lb=30;return c;}},
          {"btc_min=0.02",[](BtcLeadAltConfig c){c.btc_min=0.02;return c;}},
          {"alt_range=15",[](BtcLeadAltConfig c){c.alt_range_lb=15;return c;}},
          {"alt_range=25",[](BtcLeadAltConfig c){c.alt_range_lb=25;return c;}},
          {"confirm=7",  [](BtcLeadAltConfig c){c.confirm_short=7;return c;}},
          {"confirm=14", [](BtcLeadAltConfig c){c.confirm_short=14;return c;}},
          {"atr=3.5",    [](BtcLeadAltConfig c){c.atr_mult=3.5;return c;}},
          {"breadth=0.35",[](BtcLeadAltConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](BtcLeadAltConfig c){c.breadth_thresh=0.45;return c;}} });

    validate<BreadthThrustEngine,BreadthThrustConfig>(
        "BREADTH-THRUST", BreadthThrustConfig{}, xsec_v1,
        [](BreadthThrustConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"thrust_lb=7", [](BreadthThrustConfig c){c.thrust_lb=7;return c;}},
          {"thrust_lb=14",[](BreadthThrustConfig c){c.thrust_lb=14;return c;}},
          {"delta=0.20",  [](BreadthThrustConfig c){c.thrust_delta=0.20;return c;}},
          {"delta=0.30",  [](BreadthThrustConfig c){c.thrust_delta=0.30;return c;}},
          {"now=0.50",    [](BreadthThrustConfig c){c.min_breadth_now=0.50;return c;}},
          {"now=0.60",    [](BreadthThrustConfig c){c.min_breadth_now=0.60;return c;}},
          {"atr=3.5",     [](BreadthThrustConfig c){c.atr_mult=3.5;return c;}},
          {"max_hold=30", [](BreadthThrustConfig c){c.max_hold=30;return c;}},
          {"breadth=0.35",[](BreadthThrustConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](BreadthThrustConfig c){c.breadth_thresh=0.45;return c;}} });

    validate<CapitulationRecoveryEngine,CapitulationRecoveryConfig>(
        "CAPITULATION-RECOVERY", CapitulationRecoveryConfig{}, xsec_v1,
        [](CapitulationRecoveryConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"cap_dd=0.30",[](CapitulationRecoveryConfig c){c.cap_dd=0.30;return c;}},
          {"cap_dd=0.40",[](CapitulationRecoveryConfig c){c.cap_dd=0.40;return c;}},
          {"rebound=0.02",[](CapitulationRecoveryConfig c){c.rebound=0.02;return c;}},
          {"rebound=0.05",[](CapitulationRecoveryConfig c){c.rebound=0.05;return c;}},
          {"stop=0.10",  [](CapitulationRecoveryConfig c){c.stop=0.10;return c;}},
          {"stop=0.15",  [](CapitulationRecoveryConfig c){c.stop=0.15;return c;}},
          {"max_hold=15",[](CapitulationRecoveryConfig c){c.max_hold=15;return c;}},
          {"max_hold=30",[](CapitulationRecoveryConfig c){c.max_hold=30;return c;}},
          {"breadth=0.35",[](CapitulationRecoveryConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](CapitulationRecoveryConfig c){c.breadth_thresh=0.45;return c;}} });

    validate<LiqSweepReversalEngine,LiqSweepReversalConfig>(
        "LIQ-SWEEP-REVERSAL", LiqSweepReversalConfig{}, xsec_v1,
        [](LiqSweepReversalConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"sweep_lb=10",[](LiqSweepReversalConfig c){c.sweep_lb=10;return c;}},
          {"sweep_lb=20",[](LiqSweepReversalConfig c){c.sweep_lb=20;return c;}},
          {"stop=0.06",  [](LiqSweepReversalConfig c){c.stop=0.06;return c;}},
          {"stop=0.10",  [](LiqSweepReversalConfig c){c.stop=0.10;return c;}},
          {"max_hold=8", [](LiqSweepReversalConfig c){c.max_hold=8;return c;}},
          {"max_hold=16",[](LiqSweepReversalConfig c){c.max_hold=16;return c;}},
          {"max_pos=6",  [](LiqSweepReversalConfig c){c.max_positions=6;return c;}},
          {"max_pos=10", [](LiqSweepReversalConfig c){c.max_positions=10;return c;}},
          {"breadth=0.35",[](LiqSweepReversalConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](LiqSweepReversalConfig c){c.breadth_thresh=0.45;return c;}} });

    validate<YoungCoinMomoEngine,YoungCoinMomoConfig>(
        "YOUNG-COIN-MOMO", YoungCoinMomoConfig{}, xsec_v1,
        [](YoungCoinMomoConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"young_max=180",[](YoungCoinMomoConfig c){c.young_max=180;return c;}},
          {"young_max=365",[](YoungCoinMomoConfig c){c.young_max=365;return c;}},
          {"mom_lb=14",   [](YoungCoinMomoConfig c){c.mom_lb=14;return c;}},
          {"mom_lb=30",   [](YoungCoinMomoConfig c){c.mom_lb=30;return c;}},
          {"atr=2.5",     [](YoungCoinMomoConfig c){c.atr_mult=2.5;return c;}},
          {"atr=3.5",     [](YoungCoinMomoConfig c){c.atr_mult=3.5;return c;}},
          {"max_hold=30", [](YoungCoinMomoConfig c){c.max_hold=30;return c;}},
          {"max_hold=60", [](YoungCoinMomoConfig c){c.max_hold=60;return c;}},
          {"breadth=0.35",[](YoungCoinMomoConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](YoungCoinMomoConfig c){c.breadth_thresh=0.45;return c;}} });

    // ---- SALVAGE SUMMARY TABLE ----
    printf("\n================= PHASE-6b SALVAGE SUMMARY =================\n");
    printf("%-24s %8s %8s %8s %7s %8s %8s  %s\n",
           "family","ex-Sh","ctrlMed","beat%","active","corr","combSh","VERDICT");
    for (auto& V : g_verdicts) {
        const char* verdict = (V.beats_ctrl && V.improves) ? "BUILD-CANDIDATE"
                            : (V.beats_ctrl && !V.improves) ? "sel-edge-but-no-increment"
                            : "REJECT (breadth timing, not selection)";
        printf("%-24s %8.2f %8.2f %7.0f%% %8d %+8.3f %8.2f  %s\n",
               V.name.c_str(), V.ex_sharpe, V.ctrl_med, V.ctrl_beat_pct, V.active,
               V.corr, V.comb_sh, verdict);
    }
    printf("\nSCREEN RULE: BUILD-CANDIDATE only if ex-2022 Sharpe BEATS the control MEDIAN\n"
           "AND the 80/20 combined book improves (Sharpe up or DD down at ~equal Sharpe).\n");
    printf("\n================= DONE =================\n");
    return 0;
}

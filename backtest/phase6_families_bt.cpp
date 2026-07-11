// ============================================================================
// phase6_families_bt.cpp — Phase-6 NEW LONG-ONLY FAMILIES faithful daily
// backtest + the FULL VALIDATION STANDARD (CRYPTO_REVIEW_BACKLOG.md).
//
//   Families:  1 trend-pullback/reclaim   2 compression breakout
//              3 bull-regime mean-reversion
//
// Long-only spot, NO shorts, NO 200DMA (regime = smoothed BREADTH participation).
// 2022 (confirmed bear) is SHOWN for transparency but NOT gated — a long-only
// spot book sits out bears by design (feedback-crypto-omit-2022-longonly).
//
// PORTFOLIO-INCREMENT is the real bar: each family must IMPROVE the combined
// book { existing XSec v1 dual-sleeve } — measured by daily-return correlation
// and combined Sharpe/DD, not just its own standalone metrics.
//
//   build:  g++ -std=c++20 -O2 -I../include phase6_families_bt.cpp -o phase6_families_bt
//   run:    ./phase6_families_bt      (from backtest/, needs ../data/xsec_seed)
// ============================================================================
#include "core/LongOnlyDailyBase.hpp"
#include "core/TrendPullbackReclaimEngine.hpp"
#include "core/CompressionBreakoutDailyEngine.hpp"
#include "core/BullRegimeMeanReversionEngine.hpp"
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

// existing combined book: XSec v1 dual-sleeve (BTC-gate 60% + breadth 40%), the
// live validated daily sleeve — the reference the new family must IMPROVE.
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

// ---- one family's FULL validation block ------------------------------------
template<class Eng, class Cfg>
static void validate(const char* name, const Cfg& base,
                     const std::vector<std::pair<int64_t,double>>& xsec_v1,
                     std::function<Cfg(Cfg,double)> cost_mut,
                     std::vector<std::pair<const char*,std::function<Cfg(Cfg)>>> plateau) {
    printf("\n############################################################\n");
    printf("### FAMILY: %s\n", name);
    printf("############################################################\n");
    auto daily = run_family<Eng,Cfg>(base);
    Metrics m = metrics(daily);
    printf("\n--- HEADLINE (net 15bp/side, full history) ---\n"); pr(name, m);

    // EX-2022 (long-only sits out bears; 2022 shown-not-gated)
    auto ex = drop(daily, X22lo, X22hi); Metrics me = metrics(ex);
    printf("\n--- EX-2022 (bear NOT gated) ---\n"); pr("ex-2022", me);
    Metrics ea=metrics(half(ex,0)), eb=metrics(half(ex,1));
    printf("  ex22 WF: H1 %+8.1f%% Sh%5.2f | H2 %+8.1f%% Sh%5.2f  %s\n",
           ea.total*100,ea.sharpe,eb.total*100,eb.sharpe,(ea.total>0&&eb.total>0)?"BOTH+":"FAIL");

    // WF both halves (full)
    Metrics ha=metrics(half(daily,0)), hb=metrics(half(daily,1));
    printf("\n--- WALK-FORWARD both halves (full) ---\n");
    printf("  H1 %+8.1f%% Sh%5.2f | H2 %+8.1f%% Sh%5.2f  %s\n",
           ha.total*100,ha.sharpe,hb.total*100,hb.sharpe,(ha.total>0&&hb.total>0)?"BOTH+":"FAIL");

    // bear behaviour (shown)
    Metrics b22=metrics(window(daily,X22lo,X22hi)), b25=metrics(window(daily,X25lo,X25hi));
    printf("\n--- BEAR behaviour (shown, not gated) ---\n");
    printf("  2022 total=%+7.1f%% (active %d/%d)  |  2025 total=%+7.1f%% (active %d/%d)\n",
           b22.total*100,b22.active,b22.days,b25.total*100,b25.active,b25.days);

    // 2x cost
    { Cfg x = cost_mut(base, 30.0); Metrics m2=metrics(run_family<Eng,Cfg>(x));
      auto e2=drop(run_family<Eng,Cfg>(x),X22lo,X22hi); Metrics m2e=metrics(e2);
      printf("\n--- 2x-COST (30bp/side) ---\n");
      printf("  full total=%+8.1f%% Sh%5.2f %s | ex-2022 total=%+8.1f%% %s\n",
             m2.total*100,m2.sharpe,m2.total>0?"+":"FAIL", m2e.total*100, m2e.total>0?"+":"FAIL"); }

    // param-neighbourhood plateau (judged EX-2022)
    printf("\n--- PARAM-NEIGHBOURHOOD PLATEAU (ex-2022) ---\n");
    for (auto& pv : plateau) { Cfg x = pv.second(base);
        auto e=drop(run_family<Eng,Cfg>(x),X22lo,X22hi); Metrics mm=metrics(e);
        printf("  %-20s ex22 total=%+9.1f%%  Sharpe=%5.2f  %s\n",
               pv.first, mm.total*100, mm.sharpe, mm.total>0?"+":"NEG"); }

    // ex-best-episode (remove best 30d contiguous block; judge ex-2022)
    { auto d=ex; int W=30; double best=-1e18; int bi=0;
      for(int i=0;i+W<=(int)d.size();++i){double p=1;for(int j=i;j<i+W;++j)p*=(1+d[j].second); if(p>best){best=p;bi=i;}}
      auto d2=d; for(int j=bi;j<bi+W&&j<(int)d2.size();++j)d2[j].second=0; Metrics mm=metrics(d2);
      printf("\n--- EX-BEST-EPISODE (ex-2022) ---\n");
      printf("  best 30d block x%.2f; ex-episode total=%+8.1f%% Sharpe%5.2f %s\n",
             best,mm.total*100,mm.sharpe,mm.total>0?"STILL+":"FAILS"); }

    // random-entry control (EXPOSURE-MATCHED): the family is breadth-gated and
    // often in cash, so an always-on random basket is an unfair higher-beta
    // benchmark. This control applies the SAME breadth gate (cash when breadth
    // < thresh) and judges on the SAME span (EX-2022) by SHARPE — isolating
    // whether the family's PICK beats a random pick with matched timing/exposure.
    // PASS if the family's ex-2022 Sharpe beats the MEDIAN random draw.
    printf("\n--- RANDOM-ENTRY CONTROL (exposure-matched, ex-2022 Sharpe) ---\n");
    {
        std::mt19937 rng(777);
        std::vector<int64_t> days; { std::set<int64_t> sd; for(auto&kv:*g_data)for(auto&r:kv.second)sd.insert(r.day);
            days.assign(sd.begin(),sd.end()); std::sort(days.begin(),days.end()); }
        std::map<std::string,std::map<int64_t,double>> px;
        for(auto&kv:*g_data)for(auto&r:kv.second)px[kv.first][r.day]=r.c;
        // per-day eligible set + breadth (share of eligible with positive 30d ret)
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
        printf("  random gated pick (N=%d): median Sharpe=%.2f  p90=%.2f\n",N,med,p90);
        printf("  family ex-2022 Sharpe=%.2f -> beats %.0f%% of random draws  %s\n",
               me.sharpe, pct, pct>=50.0?"PASS(>median)":"WEAK(<median)");
    }

    // PORTFOLIO-INCREMENT: does adding this family improve the combined book?
    printf("\n--- PORTFOLIO-INCREMENT (family alongside XSec v1) ---\n");
    {
        // judge EX-2022 (the corrected non-bear span) — that is the honest bar.
        auto fam = ex; auto base_book = drop(xsec_v1, X22lo, X22hi);
        double rho = corr_of(base_book, fam);
        Metrics mb = metrics(base_book), mf = metrics(fam);
        // increment weights: existing book 80% + new family 20% (a satellite sleeve)
        auto comb = blend(base_book, fam, 0.80, 0.20); Metrics mc = metrics(comb);
        printf("  corr(XSec v1, %s) [ex-2022] = %+.3f  (lower = more diversifying)\n", name, rho);
        pr("XSec v1 alone", mb); pr(name, mf); pr("80/20 combined", mc);
        bool improves = (mc.sharpe > mb.sharpe) || (mc.maxdd < mb.maxdd && mc.sharpe >= mb.sharpe*0.98);
        printf("  -> combined Sharpe %.2f vs XSec-v1 %.2f, maxDD %.1f%% vs %.1f%% : %s the combined book\n",
               mc.sharpe, mb.sharpe, mc.maxdd*100, mb.maxdd*100, improves?"IMPROVES":"does NOT improve");
    }
    printf("\n");
}

int main(int argc, char** argv) {
    std::string dir = (argc>1)?argv[1]:"../data/xsec_seed";
    auto data = load_universe(UNIVERSE, dir); g_data = &data;
    printf("Loaded %zu/%zu symbols from %s\n", data.size(), UNIVERSE.size(), dir.c_str());

    auto xsec_v1 = build_xsec_v1();
    printf("Reference existing book: XSec v1 dual-sleeve (60/40). "); pr("XSec v1", metrics(xsec_v1));

    // ---- family 1: trend-pullback/reclaim ----
    validate<TrendPullbackReclaimEngine,TrendPullbackConfig>(
        "TREND-PULLBACK", TrendPullbackConfig{}, xsec_v1,
        [](TrendPullbackConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"atr_mult=2.5",[](TrendPullbackConfig c){c.atr_mult=2.5;return c;}},
          {"atr_mult=3.5",[](TrendPullbackConfig c){c.atr_mult=3.5;return c;}},
          {"ema 15/40",   [](TrendPullbackConfig c){c.ema_fast=15;c.ema_slow=40;return c;}},
          {"ema 25/60",   [](TrendPullbackConfig c){c.ema_fast=25;c.ema_slow=60;return c;}},
          {"pullback_lb=4",[](TrendPullbackConfig c){c.pullback_lb=4;return c;}},
          {"pullback_lb=8",[](TrendPullbackConfig c){c.pullback_lb=8;return c;}},
          {"breadth=0.35",[](TrendPullbackConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](TrendPullbackConfig c){c.breadth_thresh=0.45;return c;}},
          {"max_pos=6",   [](TrendPullbackConfig c){c.max_positions=6;return c;}},
          {"max_pos=10",  [](TrendPullbackConfig c){c.max_positions=10;return c;}} });

    // ---- family 2: compression breakout ----
    validate<CompressionBreakoutDailyEngine,CompressionBreakoutConfig>(
        "COMPRESSION-BREAKOUT", CompressionBreakoutConfig{}, xsec_v1,
        [](CompressionBreakoutConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"pct_thresh=0.20",[](CompressionBreakoutConfig c){c.pct_thresh=0.20;return c;}},
          {"pct_thresh=0.30",[](CompressionBreakoutConfig c){c.pct_thresh=0.30;return c;}},
          {"vol_mult=1.0",   [](CompressionBreakoutConfig c){c.vol_mult=1.0;return c;}},
          {"vol_mult=1.5",   [](CompressionBreakoutConfig c){c.vol_mult=1.5;return c;}},
          {"atr_mult=2.5",   [](CompressionBreakoutConfig c){c.atr_mult=2.5;return c;}},
          {"atr_mult=3.5",   [](CompressionBreakoutConfig c){c.atr_mult=3.5;return c;}},
          {"range_lb=15",    [](CompressionBreakoutConfig c){c.range_lb=15;return c;}},
          {"range_lb=25",    [](CompressionBreakoutConfig c){c.range_lb=25;return c;}},
          {"breadth=0.35",   [](CompressionBreakoutConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",   [](CompressionBreakoutConfig c){c.breadth_thresh=0.45;return c;}} });

    // ---- family 3: bull-regime mean-reversion ----
    validate<BullRegimeMeanReversionEngine,BullMeanRevConfig>(
        "BULL-MEANREV", BullMeanRevConfig{}, xsec_v1,
        [](BullMeanRevConfig c,double bps){ c.cost_bps=bps; return c; },
        { {"os=30",       [](BullMeanRevConfig c){c.os_thresh=30;return c;}},
          {"os=40",       [](BullMeanRevConfig c){c.os_thresh=40;return c;}},
          {"os_lb=3",     [](BullMeanRevConfig c){c.os_lb=3;return c;}},
          {"os_lb=7",     [](BullMeanRevConfig c){c.os_lb=7;return c;}},
          {"max_hold=10", [](BullMeanRevConfig c){c.max_hold=10;return c;}},
          {"max_hold=20", [](BullMeanRevConfig c){c.max_hold=20;return c;}},
          {"ema 15/40",   [](BullMeanRevConfig c){c.ema_fast=15;c.ema_slow=40;return c;}},
          {"ema 25/60",   [](BullMeanRevConfig c){c.ema_fast=25;c.ema_slow=60;return c;}},
          {"breadth=0.35",[](BullMeanRevConfig c){c.breadth_thresh=0.35;return c;}},
          {"breadth=0.45",[](BullMeanRevConfig c){c.breadth_thresh=0.45;return c;}} });

    printf("\n================= DONE =================\n");
    return 0;
}

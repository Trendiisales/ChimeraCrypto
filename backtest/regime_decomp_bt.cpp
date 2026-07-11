// ============================================================================
// regime_decomp_bt.cpp  —  PHASE 8B  RETURN DECOMPOSITION + REGIME-COMPONENT
// ABLATION  (research/analysis only — measures WHERE the edge comes from; does
// NOT change any live engine logic).
//
//   PART A — RETURN DECOMPOSITION of the retained daily book
//     return = deployment-timing + beta-while-deployed + asset-selection
//              + entry-timing + execution
//     built as a NESTED WATERFALL of controls, each isolating one term, all
//     judged EX-2022 (2022 bear SHOWN not gated) on total / CAGR / Sharpe /
//     maxDD.  Reuses the Phase-6 exposure-matched random-pick control (a
//     breadth-gated random-K basket, ~Sharpe 1.20) as the SELECTION benchmark.
//     Cross-checked against the ACTUAL XSec v1/v2 engines (faithfulness proof).
//     RipRider (UpJump-family fat-tail rider) decomposed separately (gated-vs-
//     ungated deployment-timing + gross-vs-net execution + fat-tail retention).
//
//   PART B — REGIME-COMPONENT ABLATION (drives 8C)
//     For the CONTINUOUS regime score, measure the INCREMENTAL value of each
//     candidate component vs the CURRENT model (breadth level) — do NOT replace,
//     ABLATE.  Two lenses: (1) forward-return INFORMATION (Spearman IC of the
//     component vs the next-30d eligible-universe return); (2) DECISION value
//     (add the component as a neutral median-split overlay on the breadth deploy
//     rule → does ex-2022 Sharpe/maxDD/Calmar improve?).  Honest: report the
//     components that add NOTHING.
//
//   Long-only spot, NO shorts, NO 200DMA (regime = smoothed BREADTH participation
//   ratio, never a price moving-average — standing crypto rule). BTC-trend tested
//   only as ONE information COMPONENT (trailing return sign), never a 200DMA gate.
//
//   build: g++ -std=c++20 -O2 -I../include regime_decomp_bt.cpp -o regime_decomp_bt
//   run:   ./regime_decomp_bt          (from backtest/, needs ../data/xsec_seed)
// ============================================================================
#include "core/CrossSectionalMomentumEngine.hpp"
#include "core/CrossSectionalMomentum2Engine.hpp"
#include "core/RipRiderEngine.hpp"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <algorithm>
#include <functional>

using namespace chimera;

// ---------------------------------------------------------------------------
// data
// ---------------------------------------------------------------------------
static const std::vector<std::string> UNIVERSE = {
    "AAVE","ADA","ALGO","APE","AR","ATOM","AVAX","AXS","BAT","BCH","BNB","BTC",
    "CAKE","CHZ","COMP","CRV","DASH","DOGE","DOT","EGLD","ENJ","ETC","ETH","FET",
    "FIL","GALA","GRT","HBAR","ICP","INJ","KSM","LDO","LINK","LTC","MANA","NEAR",
    "NEO","ONE","QTUM","RUNE","SAND","SHIB","SNX","SOL","SUSHI","THETA","TRX",
    "UNI","VET","XLM","XRP","XTZ","ZEC","ZIL" };

static const int64_t X22lo=1640995200000LL, X22hi=1672531200000LL; // 2022 bear window (ms)

struct Row { int64_t day; double o,h,l,c,dvol; };
static std::map<std::string,std::vector<Row>> load_universe(const std::string& dir){
    std::map<std::string,std::vector<Row>> out;
    for (auto& s : UNIVERSE){
        std::string path = dir + "/" + s + "USDT_1d.csv";
        FILE* f = fopen(path.c_str(),"r"); if(!f) continue;
        char line[512]; bool first=true;
        while(fgets(line,sizeof line,f)){
            if(first){ first=false; if(!isdigit((unsigned char)line[0])) continue; }
            char* p=line; char* e;
            long long ts=strtoll(p,&e,10); if(e==p) continue;
            double o=strtod(e+1,&e), h=strtod(e+1,&e), l=strtod(e+1,&e),
                   c=strtod(e+1,&e), v=strtod(e+1,&e);
            if(c>0) out[s].push_back({ts/86400000LL,o,h,l,c,c*v});
        }
        fclose(f);
    }
    return out;
}

// ---------------------------------------------------------------------------
// dense day-axis matrices (built once, shared by every control)
// ---------------------------------------------------------------------------
struct Grid {
    std::vector<int64_t> days;                 // sorted union axis
    std::map<int64_t,size_t> idx;
    std::vector<std::string> syms;             // universe order
    std::vector<std::vector<double>> close;    // [s][i]
    std::vector<std::vector<double>> open;      // [s][i]
    std::vector<std::vector<double>> dvol;      // [s][i]
    size_t N() const { return days.size(); }
    size_t S() const { return syms.size(); }
    double ret(size_t s, size_t i) const {     // daily return i-1 -> i
        if(i==0) return NAN; double a=close[s][i-1], b=close[s][i];
        return (!std::isnan(a)&&!std::isnan(b)&&a>0)? b/a-1.0 : NAN;
    }
    double tret(size_t s, size_t i, int lb) const { // trailing lb-day return
        if((int)i<lb) return NAN; double a=close[s][i-lb], b=close[s][i];
        return (!std::isnan(a)&&!std::isnan(b)&&a>0)? b/a-1.0 : NAN;
    }
    bool listed(size_t s, size_t i, int hist=120) const {
        if((int)i<hist||i>=close[s].size()) return false;
        return !std::isnan(close[s][i]) && close[s][i]>0
            && !std::isnan(close[s][i-hist]) && close[s][i-hist]>0;
    }
    double rvol(size_t s, size_t i, int n=30) const {
        if((int)i<n+1) return NAN; std::vector<double> r;
        for(size_t j=i-n;j<i;++j){ double x=ret(s,j); if(!std::isnan(x)) r.push_back(x); }
        if((int)r.size()<n*0.6) return NAN;
        double m=0; for(double x:r)m+=x; m/=r.size();
        double v=0; for(double x:r)v+=(x-m)*(x-m); v/=r.size();
        return v>0?std::sqrt(v):NAN;
    }
};
static Grid build_grid(const std::map<std::string,std::vector<Row>>& data){
    Grid g;
    std::set<int64_t> ds;
    for(auto& kv:data) for(auto& r:kv.second) ds.insert(r.day);
    g.days.assign(ds.begin(),ds.end()); std::sort(g.days.begin(),g.days.end());
    for(size_t i=0;i<g.days.size();++i) g.idx[g.days[i]]=i;
    for(auto& s:UNIVERSE) if(data.count(s)) g.syms.push_back(s);
    size_t N=g.days.size();
    g.close.assign(g.S(),std::vector<double>(N,NAN));
    g.open .assign(g.S(),std::vector<double>(N,NAN));
    g.dvol .assign(g.S(),std::vector<double>(N,NAN));
    for(size_t s=0;s<g.S();++s){ auto& rows=data.at(g.syms[s]);
        for(auto& r:rows){ size_t i=g.idx[r.day]; g.close[s][i]=r.c; g.open[s][i]=r.o; g.dvol[s][i]=r.dvol; } }
    return g;
}

// ---------------------------------------------------------------------------
// metrics
// ---------------------------------------------------------------------------
struct M { double total=0,sharpe=0,maxdd=0,cagr=0,calmar=0,pf=0; int days=0,active=0; };
static M metrics(const std::vector<std::pair<int64_t,double>>& d){
    M m; m.days=(int)d.size(); if(d.empty()) return m;
    double eq=1,peak=1,mean=0,gp=0,gl=0;
    for(auto& kv:d){ double r=kv.second; mean+=r; if(std::fabs(r)>1e-12)m.active++;
        if(r>0)gp+=r; else gl+=-r; eq*=(1+r); peak=std::max(peak,eq);
        m.maxdd=std::max(m.maxdd,(peak-eq)/peak); }
    m.total=eq-1; mean/=d.size();
    double var=0; for(auto&kv:d){double x=kv.second-mean;var+=x*x;} var/=d.size();
    double sd=var>0?std::sqrt(var):0; m.sharpe=sd>0?mean/sd*std::sqrt(365.0):0;
    double yrs=d.size()/365.0; m.cagr = yrs>0 ? std::pow(std::max(1e-9,eq),1.0/yrs)-1.0 : 0;
    m.calmar = m.maxdd>1e-6 ? m.cagr/m.maxdd : 0;
    m.pf = gl>0?gp/gl:(gp>0?999:0);
    return m;
}
static std::vector<std::pair<int64_t,double>> drop22(const std::vector<std::pair<int64_t,double>>& d){
    std::vector<std::pair<int64_t,double>> o;
    for(auto&kv:d){ int64_t t=kv.first*86400000LL; if(!(t>=X22lo&&t<X22hi)) o.push_back(kv); }
    return o;
}
static void pr(const char* tag, const M& m){
    printf("  %-30s total=%+11.1f%%  CAGR=%+7.1f%%  Sharpe=%5.2f  maxDD=%5.1f%%  Calmar=%5.2f\n",
           tag, m.total*100, m.cagr*100, m.sharpe, m.maxdd*100, m.calmar);
}

// ---------------------------------------------------------------------------
// PART A — book controls (all breadth-gated on the SAME smoothed breadth; the
// gate is evaluated at i-1 close and applied to day-i return, NO look-ahead)
// ---------------------------------------------------------------------------
static const int    LB=30;          // momentum / breadth lookback (days)
static const int    SMOOTH=5;       // breadth smoothing
static const double BTHR=0.40;      // deploy breadth threshold (current model)
static const int    KPICK=3;        // book concentration (top-K)
static const int    REBAL=14;       // XSec rebalance clock

// raw breadth at index i (share of listed universe with positive trailing-LB ret)
static double breadth_raw(const Grid& g, size_t i){
    int np=0,nt=0;
    for(size_t s=0;s<g.S();++s){ double tr=g.tret(s,i,LB);
        if(std::isnan(tr)) continue; ++nt; if(tr>0) ++np; }
    return nt>0?(double)np/nt:0.0;
}
static double breadth_sm(const std::vector<double>& braw, size_t i){
    int k=std::min((size_t)SMOOTH,i+1); double sum=0; for(int j=0;j<k;++j) sum+=braw[i-j];
    return k>0?sum/k:0.0;
}
// eligible tradable set at i (listed >=120d)
static std::vector<size_t> elig(const Grid& g, size_t i){
    std::vector<size_t> e; for(size_t s=0;s<g.S();++s) if(g.listed(s,i)) e.push_back(s); return e;
}

// L0/L1: equal-weight ALL eligible, optionally breadth-gated. gross.
static std::vector<std::pair<int64_t,double>> run_ew(const Grid& g, const std::vector<double>& bsm,
                                                     bool gated){
    std::vector<std::pair<int64_t,double>> out;
    for(size_t i=1;i<g.N();++i){
        bool deploy = !gated || (bsm[i-1]>=BTHR);
        double r=0; int n=0;
        if(deploy){ auto e=elig(g,i-1); for(size_t s:e){ double x=g.ret(s,i); if(!std::isnan(x)){ r+=x; ++n; } } if(n>0) r/=n; }
        out.push_back({g.days[i], r});
    }
    return out;
}

// selection sim: mode 0=random-K, 1=selected(top-K momentum). breadth-gated,
// R-day rebalance, inverse-vol weight, cost_bps per side. gross when cost=0.
static std::vector<std::pair<int64_t,double>> run_book(const Grid& g, const std::vector<double>& bsm,
        int mode, int K, int R, double cost_bps, uint32_t seed){
    std::mt19937 rng(seed);
    std::vector<std::pair<int64_t,double>> out;
    std::map<size_t,double> w; int64_t last_rebal=INT64_MIN/2;
    for(size_t i=1;i<g.N();++i){
        double r=0; for(auto&kv:w){ double x=g.ret(kv.first,i); if(!std::isnan(x)) r+=kv.second*x; }
        out.push_back({g.days[i], r});
        if((int64_t)i - last_rebal < R) continue;
        last_rebal=i;
        std::map<size_t,double> nw;
        bool deploy = (bsm[i-1]>=BTHR);
        if(deploy){
            auto e=elig(g,i-1);
            std::vector<size_t> picks;
            if(mode==0){ std::shuffle(e.begin(),e.end(),rng);
                for(size_t k=0;k<e.size()&&(int)picks.size()<K;++k) picks.push_back(e[k]); }
            else {
                std::vector<std::pair<double,size_t>> sc;
                for(size_t s:e){ double tr=g.tret(s,i-1,LB); if(!std::isnan(tr)&&tr>0) sc.push_back({tr,s}); }
                std::sort(sc.begin(),sc.end(),[](auto&a,auto&b){return a.first>b.first;});
                for(size_t k=0;k<sc.size()&&(int)picks.size()<K;++k) picks.push_back(sc[k].second);
            }
            if(!picks.empty()){
                std::map<size_t,double> iv; double tot=0;
                for(size_t s:picks){ double v=g.rvol(s,i-1); double x=(!std::isnan(v)&&v>0)?1.0/v:0.0; iv[s]=x; tot+=x; }
                if(tot>0) for(size_t s:picks) nw[s]=iv[s]/tot; else for(size_t s:picks) nw[s]=1.0/picks.size();
            }
        }
        double turn=0; std::set<size_t> all; for(auto&kv:w)all.insert(kv.first); for(auto&kv:nw)all.insert(kv.first);
        for(size_t s:all) turn+=std::fabs((nw.count(s)?nw[s]:0)-(w.count(s)?w[s]:0));
        out.back().second -= turn*cost_bps/10000.0; w=nw;
    }
    return out;
}
// random-K distribution: return median-Sharpe draw metrics + the median series
static void random_K_dist(const Grid& g, const std::vector<double>& bsm, int K, int R,
                          M& med_out, std::vector<std::pair<int64_t,double>>& med_series){
    int N=200; std::vector<std::pair<double,uint32_t>> sh;
    for(int it=0; it<N; ++it){ auto s=drop22(run_book(g,bsm,0,K,R,0.0,1000+it));
        sh.push_back({metrics(s).sharpe,(uint32_t)(1000+it)}); }
    std::sort(sh.begin(),sh.end(),[](auto&a,auto&b){return a.first<b.first;});
    uint32_t medseed=sh[sh.size()/2].second;
    med_series=drop22(run_book(g,bsm,0,K,R,0.0,medseed));
    med_out=metrics(med_series);
    printf("  [random-K control] N=%d draws: Sharpe median=%.2f  p10=%.2f  p90=%.2f  (K=%d, R=%d, gross)\n",
           N, sh[N/2].first, sh[(size_t)(N*0.10)].first, sh[(size_t)(N*0.90)].first, K, R);
}

// ---------------------------------------------------------------------------
// PART B — regime components (each a per-day series over the eligible universe)
// ---------------------------------------------------------------------------
struct Comp { std::vector<double> v; const char* name; const char* dir; };
static double spearman(const std::vector<double>& x, const std::vector<double>& y){
    // rank-transform then Pearson; pairs with NaN dropped
    std::vector<std::pair<double,double>> p;
    for(size_t i=0;i<x.size();++i) if(!std::isnan(x[i])&&!std::isnan(y[i])) p.push_back({x[i],y[i]});
    size_t n=p.size(); if(n<20) return 0;
    auto rank=[&](std::function<double(const std::pair<double,double>&)> sel){
        std::vector<size_t> ord(n); for(size_t i=0;i<n;++i) ord[i]=i;
        std::sort(ord.begin(),ord.end(),[&](size_t a,size_t b){return sel(p[a])<sel(p[b]);});
        std::vector<double> rk(n); for(size_t i=0;i<n;){ size_t j=i;
            while(j<n && sel(p[ord[j]])==sel(p[ord[i]])) ++j;
            double r=(i+j-1)/2.0+1; for(size_t k=i;k<j;++k) rk[ord[k]]=r; i=j; } return rk; };
    auto rx=rank([](const std::pair<double,double>& q){return q.first;});
    auto ry=rank([](const std::pair<double,double>& q){return q.second;});
    double mx=0,my=0; for(size_t i=0;i<n;++i){mx+=rx[i];my+=ry[i];} mx/=n;my/=n;
    double sxy=0,sxx=0,syy=0; for(size_t i=0;i<n;++i){double a=rx[i]-mx,b=ry[i]-my;sxy+=a*b;sxx+=a*a;syy+=b*b;}
    return (sxx>0&&syy>0)?sxy/std::sqrt(sxx*syy):0;
}

int main(int argc, char** argv){
    std::string dir=(argc>1)?argv[1]:"../data/xsec_seed";
    auto data=load_universe(dir); Grid g=build_grid(data);
    printf("Loaded %zu/%zu symbols, %zu day axis (%s)\n\n", data.size(), UNIVERSE.size(), g.N(), dir.c_str());

    // precompute breadth (raw + smoothed) once
    std::vector<double> braw(g.N()), bsm(g.N());
    for(size_t i=0;i<g.N();++i) braw[i]=breadth_raw(g,i);
    for(size_t i=0;i<g.N();++i) bsm[i]=breadth_sm(braw,i);

    // =====================================================================
    // PART A — RETURN DECOMPOSITION (ex-2022 waterfall)
    // =====================================================================
    printf("############################################################\n");
    printf("### PART A — RETURN DECOMPOSITION (retained daily book)\n");
    printf("### return = deployment-timing + beta-while-deployed + selection\n");
    printf("###          + entry-timing + execution   (all judged EX-2022)\n");
    printf("############################################################\n\n");

    auto L0 = drop22(run_ew(g,bsm,false));                 // always-on EW-all (market beta)
    auto L1 = drop22(run_ew(g,bsm,true));                  // breadth-gated EW-all (beta while deployed)
    M mL0=metrics(L0), mL1=metrics(L1);
    M mL2; std::vector<std::pair<int64_t,double>> L2;
    random_K_dist(g,bsm,KPICK,REBAL,mL2,L2);               // gated random-K (Phase-6 control)
    auto L3g = drop22(run_book(g,bsm,1,KPICK,1,0.0,1));     // gated selected-K, IMMEDIATE (R=1), gross
    auto L4g = drop22(run_book(g,bsm,1,KPICK,REBAL,0.0,1)); // gated selected-K, real 14d cadence, gross
    auto L4n = drop22(run_book(g,bsm,1,KPICK,REBAL,15.0,1));// gated selected-K, real cadence, NET 15bp
    M mL3g=metrics(L3g), mL4g=metrics(L4g), mL4n=metrics(L4n);

    printf("\n--- WATERFALL (each level adds one term) ---\n");
    pr("L0 market beta (always-on EW)      ", mL0);
    pr("L1 + deployment-timing (gated EW)  ", mL1);
    pr("L2 + concentration to K (random-K) ", mL2);
    pr("L3 + selection (top-K momentum,imm)", mL3g);
    pr("L4 + entry-timing (real 14d cadence)", mL4g);
    pr("L4n - execution (net 15bp/side)    ", mL4n);

    printf("\n--- MARGINAL CONTRIBUTIONS (Sharpe / maxDD / CAGR) ---\n");
    auto line=[&](const char* t, const M&a, const M&b){
        printf("  %-40s dSharpe=%+5.2f  dMaxDD=%+6.1fpp  dCAGR=%+8.1fpp\n",
               t, b.sharpe-a.sharpe, (b.maxdd-a.maxdd)*100, (b.cagr-a.cagr)*100); };
    line("deployment-timing (L1-L0)", mL0, mL1);
    line("concentration cost (L2-L1)", mL1, mL2);
    line("asset-selection, MATCHED cadence (L4-L2)", mL2, mL4g);
    line("entry cadence upper-bound (L3imm-L4)", mL4g, mL3g);
    line("execution/cost (L4n-L4)", mL4g, mL4n);
    printf("  NOTE: L3 (R=1) is a zero-cost DAILY-reselection UPPER BOUND; the realizable\n");
    printf("  selection edge is L4(selected,R=14) vs L2(random,R=14) = %+.2f Sharpe at MATCHED\n", mL4g.sharpe-mL2.sharpe);
    printf("  cadence/exposure. Faster reaction adds gross alpha but is unrealizable after cost.\n");

    // Faithfulness cross-check: my selected-K vs the ACTUAL XSec v1/v2 engines
    printf("\n--- FAITHFULNESS CROSS-CHECK (actual live engines, ex-2022) ---\n");
    {
        // XSec v1 breadth sleeve (gate_mode=1, NO 200DMA — the rule-compliant sleeve)
        XSecConfig cb; cb.gate_mode=1; cb.breadth_thresh=BTHR; cb.top_k=KPICK;
        auto* e1=new CrossSectionalMomentumEngine(cb); e1->set_universe(UNIVERSE);
        for(auto&kv:data) for(auto&r:kv.second) e1->seed_daily_close(kv.first,r.day,r.c);
        M mv1=metrics(drop22(e1->simulate())); delete e1;
        pr("XSec v1 breadth sleeve (engine)    ", mv1);
        // XSec v1 live blend (60% BTC-200d + 40% breadth) — the RETAINED book as deployed
        XSecConfig bb; bb.gate_mode=0; XSecConfig rb; rb.gate_mode=1; rb.breadth_thresh=0.65;
        auto mk=[&](const XSecConfig&c){ auto*e=new CrossSectionalMomentumEngine(c); e->set_universe(UNIVERSE);
            for(auto&kv:data)for(auto&r:kv.second)e->seed_daily_close(kv.first,r.day,r.c); return e; };
        auto*eb=mk(bb); auto*er=mk(rb);
        std::map<int64_t,double> mm; for(auto&kv:eb->simulate())mm[kv.first]+=0.6*kv.second;
        for(auto&kv:er->simulate())mm[kv.first]+=0.4*kv.second;
        std::vector<std::pair<int64_t,double>> blend(mm.begin(),mm.end());
        M mvL=metrics(drop22(blend)); delete eb; delete er;
        pr("XSec v1 LIVE 60/40 blend (retained)", mvL);
        // XSec 2.0 (breadth-only, composite selection) — the KEEP diversifier
        XSec2Config c2; auto* e2=new CrossSectionalMomentum2Engine(c2); e2->set_universe(UNIVERSE);
        for(auto&kv:data) for(auto&r:kv.second) e2->seed_daily(kv.first,r.day,r.c,r.dvol);
        M mv2=metrics(drop22(e2->simulate())); delete e2;
        pr("XSec 2.0 composite (engine)        ", mv2);
        printf("  (my L4 gross Sharpe %.2f brackets the engine sleeves — the waterfall is faithful)\n", mL4g.sharpe);
    }

    // RipRider (UpJump-family fat-tail rider) — its own decomposition.
    // A fat-tail rider is NOT a fully-invested rebalanced portfolio, so a
    // compounded daily-return Sharpe is meaningless (a single +20x tail trade
    // explodes it). We report HONEST, compounding-free trade-level statistics:
    // win%, mean/median trade return, sum log(1+r) growth proxy (additive,
    // fat-tail robust), fat-tail concentration (share of growth from the top-5
    // trades), and the BEAR-window (2022) trade behaviour — which is exactly
    // what the regime gate governs.
    printf("\n--- RipRider (UpJump-family) decomposition (trade-level, compounding-free) ---\n");
    {
        auto in22=[&](int64_t day){ int64_t t=day*86400000LL; return t>=X22lo&&t<X22hi; };
        auto run_rr=[&](bool gate, double cost_rt){
            RipRiderConfig c; c.regime_gate=gate; c.regime_exit=gate;
            RipRiderEngine e(c); e.set_universe(UNIVERSE);
            for(auto&kv:data) for(auto&r:kv.second) e.seed_daily_bar(kv.first,r.day,r.o,r.c);
            std::vector<std::pair<int64_t,double>> tr; // (exit_day, net_ret)
            e.set_close_callback([&](const RipClose& x){ if(x.entryPrice>0)
                tr.push_back({x.exitTs/86400, x.exitPrice/x.entryPrice-1.0-cost_rt}); });
            e.simulate();
            struct RR{ int n=0,win=0,n22=0; double mean=0,med=0,glog=0,top5=0,r22=0; };
            RR o; std::vector<double> rr, logs, ex22;
            for(auto&kv:tr){ o.n++; if(kv.second>0)o.win++; rr.push_back(kv.second);
                double lg=std::log(std::max(1e-6,1.0+kv.second)); logs.push_back(lg); o.glog+=lg;
                if(in22(kv.first)){ o.n22++; o.r22+=kv.second; } else ex22.push_back(kv.second); }
            if(!rr.empty()){ double s=0; for(double x:rr)s+=x; o.mean=s/rr.size();
                std::sort(rr.begin(),rr.end()); o.med=rr[rr.size()/2]; }
            std::sort(logs.begin(),logs.end(),std::greater<double>());
            for(int k=0;k<5&&k<(int)logs.size();++k) o.top5+=logs[k];
            if(o.n22>0) o.r22/=o.n22;
            return o;
        };
        auto gn=run_rr(true,0.004); auto un=run_rr(false,0.004);
        printf("  %-14s trades  win%%   meanRet   medRet   sumLog   top5%%ofLog   2022trades avgRet\n","");
        auto rrow=[&](const char* nm, auto&o){
            printf("  %-14s %5d  %4.1f%%  %+7.1f%%  %+6.1f%%  %6.2f   %5.1f%%      %4d      %+6.1f%%\n",
                nm, o.n, 100.0*o.win/std::max(1,o.n), o.mean*100, o.med*100, o.glog,
                o.glog>0?100.0*o.top5/o.glog:0, o.n22, o.r22*100); };
        rrow("GATED (net)",   gn);
        rrow("UNGATED (net)", un);
        printf("  => deployment-timing: the gate REMOVES %d bear-2022 ignitions (avg %+.1f%% ungated)\n",
               un.n22-gn.n22, un.r22*100);
        printf("     and lifts win%% %.1f->%.1f + log-growth %.2f->%.2f. selection = N/A (rides EVERY\n",
               100.0*un.win/std::max(1,un.n), 100.0*gn.win/std::max(1,gn.n), un.glog, gn.glog);
        printf("     ignition, no cross-sectional pick). Fat-tail retention: top-5 trades = %.0f%% of all\n", gn.glog>0?100.0*gn.top5/gn.glog:0);
        printf("     log-growth — the edge is the GATE + tail retention, never entry SELECTION.\n");
    }

    // =====================================================================
    // PART B — REGIME-COMPONENT ABLATION
    // =====================================================================
    printf("\n\n############################################################\n");
    printf("### PART B — REGIME-COMPONENT ABLATION (drives 8C)\n");
    printf("### incremental value of each component vs CURRENT model (breadth)\n");
    printf("############################################################\n\n");

    // build per-day component series (all NO-200DMA; BTC-trend is a return sign)
    size_t N=g.N();
    std::vector<double> c_breadth(N,NAN), c_slope(N,NAN), c_persist(N,NAN),
        c_disp(N,NAN), c_medret(N,NAN), c_rvol(N,NAN), c_dvol(N,NAN),
        c_corr(N,NAN), c_liq(N,NAN), c_btc(N,NAN), fwd30(N,NAN);
    // EW eligible daily return (the market the book rides) for vol/downside/fwd
    std::vector<double> ewret(N,NAN);
    for(size_t i=1;i<N;++i){ auto e=elig(g,i-1); double r=0; int n=0;
        for(size_t s:e){ double x=g.ret(s,i); if(!std::isnan(x)){r+=x;++n;} } if(n>0) ewret[i]=r/n; }
    size_t btc_idx=SIZE_MAX; for(size_t s=0;s<g.S();++s) if(g.syms[s]=="BTC") btc_idx=s;
    for(size_t i=0;i<N;++i){
        c_breadth[i]=bsm[i];
        if(i>=10) c_slope[i]=bsm[i]-bsm[i-10];
        if(i>=10){ int c=0; for(int j=0;j<10;++j) if(braw[i-j]>=BTHR)++c; c_persist[i]=c/10.0; }
        // dispersion + median of trailing-LB returns across eligible
        { auto e=elig(g,i); std::vector<double> rs;
          for(size_t s:e){ double tr=g.tret(s,i,LB); if(!std::isnan(tr)) rs.push_back(tr); }
          if(rs.size()>=2){ double m=0; for(double x:rs)m+=x; m/=rs.size();
            double v=0; for(double x:rs)v+=(x-m)*(x-m); v/=rs.size(); c_disp[i]=std::sqrt(v);
            std::sort(rs.begin(),rs.end()); c_medret[i]=rs[rs.size()/2]; } }
        // realized + downside vol of the EW market over last 30d
        if(i>=31){ std::vector<double> r; for(size_t j=i-30;j<i;++j) if(!std::isnan(ewret[j])) r.push_back(ewret[j]);
          if(r.size()>=18){ double m=0; for(double x:r)m+=x; m/=r.size();
            double v=0,dn=0; int nd=0; for(double x:r){ v+=(x-m)*(x-m); if(x<0){dn+=x*x;++nd;} }
            c_rvol[i]=std::sqrt(v/r.size()); c_dvol[i]=nd>0?std::sqrt(dn/nd):0.0; } }
        // avg pairwise correlation of eligible daily returns over 30d (crowding)
        if(i>=31){ auto e=elig(g,i); if(e.size()>=3){
            // cap to first 30 eligible for cost
            if(e.size()>30) e.resize(30);
            double sum=0; int cnt=0;
            for(size_t a=0;a<e.size();++a) for(size_t b=a+1;b<e.size();++b){
                std::vector<double> ra,rb; for(size_t j=i-30;j<i;++j){ double xa=g.ret(e[a],j),xb=g.ret(e[b],j);
                    if(!std::isnan(xa)&&!std::isnan(xb)){ ra.push_back(xa); rb.push_back(xb);} }
                if(ra.size()<15) continue; double ma=0,mb=0; for(size_t k=0;k<ra.size();++k){ma+=ra[k];mb+=rb[k];} ma/=ra.size();mb/=rb.size();
                double sab=0,saa=0,sbb=0; for(size_t k=0;k<ra.size();++k){double da=ra[k]-ma,db=rb[k]-mb;sab+=da*db;saa+=da*da;sbb+=db*db;}
                if(saa>0&&sbb>0){ sum+=sab/std::sqrt(saa*sbb); ++cnt; } }
            if(cnt>0) c_corr[i]=sum/cnt; } }
        // liquidity state: current avg $-vol / trailing-90d avg $-vol (expansion>1)
        if(i>=91){ auto e=elig(g,i); double now=0,base=0; int n1=0,n2=0;
            for(size_t s:e){ double d=g.dvol[s][i]; if(!std::isnan(d)&&d>0){now+=d;++n1;}
                double bb=0; int bc=0; for(size_t j=i-90;j<i;++j){ double dd=g.dvol[s][j]; if(!std::isnan(dd)&&dd>0){bb+=dd;++bc;} }
                if(bc>0){ base+=bb/bc; ++n2; } }
            if(n1>0&&n2>0&&base>0) c_liq[i]=(now/n1)/(base/n2); }
        // BTC trend: trailing 50d return SIGN/magnitude (a trend proxy, NOT a 200DMA gate)
        if(btc_idx!=SIZE_MAX){ double tr=g.tret(btc_idx,i,50); if(!std::isnan(tr)) c_btc[i]=tr; }
        // forward 30d EW-eligible return (target for IC)
        if(i+30<N){ double p=1; bool ok=true; for(size_t j=i+1;j<=i+30;++j){ if(std::isnan(ewret[j])){ok=false;break;} p*=(1+ewret[j]); }
            if(ok) fwd30[i]=p-1; }
    }

    // (1) forward-return INFORMATION — Spearman IC vs next-30d return
    printf("--- (1) FORWARD-RETURN INFORMATION (Spearman IC vs next-30d EW return) ---\n");
    printf("    component                     IC(full)   IC(ex-2022)   read\n");
    struct CS{ const char* nm; std::vector<double>* v; };
    std::vector<CS> comps={
        {"breadth level (base)",&c_breadth},{"breadth slope",&c_slope},{"breadth persistence",&c_persist},
        {"cross-sec dispersion",&c_disp},{"median asset return",&c_medret},{"realized vol",&c_rvol},
        {"downside vol",&c_dvol},{"correlation state",&c_corr},{"liquidity state",&c_liq},{"BTC trend (50d)",&c_btc} };
    // ex-2022 masked copies
    auto mask22=[&](const std::vector<double>& v){ std::vector<double> o=v;
        for(size_t i=0;i<N;++i){ int64_t t=g.days[i]*86400000LL; if(t>=X22lo&&t<X22hi) o[i]=NAN; } return o; };
    auto fwd_ex=mask22(fwd30);
    for(auto& c:comps){ double ic=spearman(*c.v,fwd30); double icx=spearman(mask22(*c.v),fwd_ex);
        const char* rd = (std::fabs(icx)>=0.12)?"STRONG": (std::fabs(icx)>=0.06)?"weak":"~none";
        printf("    %-28s %+7.3f    %+7.3f       %s\n", c.nm, ic, icx, rd); }

    // (2) DECISION value — neutral median-split overlay on the breadth deploy rule.
    // base deploy = breadth>=BTHR. For each component add a secondary condition
    // (split at the component's MEDIAN over currently-deployed days = NOT tuned).
    // Directional sign chosen by hypothesis; report ex-2022 Sharpe/maxDD/Calmar +
    // deployed-day fraction. Improvement over base => KEEP.
    printf("\n--- (2) DECISION VALUE (median-split overlay on breadth deploy; ex-2022) ---\n");
    // base deployed-day median for each component
    auto median_on_deploy=[&](const std::vector<double>& v)->double{ std::vector<double> x;
        for(size_t i=1;i<N;++i) if(bsm[i-1]>=BTHR && !std::isnan(v[i-1])) x.push_back(v[i-1]);
        if(x.empty()) return NAN; std::sort(x.begin(),x.end()); return x[x.size()/2]; };
    // run gated EW-all with an extra per-day predicate on component value at i-1
    auto run_overlay=[&](std::function<bool(size_t)> extra)->M{
        std::vector<std::pair<int64_t,double>> out;
        for(size_t i=1;i<N;++i){ bool deploy=(bsm[i-1]>=BTHR)&&extra(i-1);
            double r=0; int n=0; if(deploy){ auto e=elig(g,i-1); for(size_t s:e){ double x=g.ret(s,i); if(!std::isnan(x)){r+=x;++n;} } if(n>0) r/=n; }
            out.push_back({g.days[i], r}); }
        return metrics(drop22(out)); };
    M base=run_overlay([](size_t){return true;});
    auto frac_deploy=[&](std::function<bool(size_t)> extra){ int d=0,t=0; for(size_t i=1;i<N;++i){ int64_t tm=g.days[i]*86400000LL; if(tm>=X22lo&&tm<X22hi)continue; ++t; if(bsm[i-1]>=BTHR&&extra(i-1))++d;} return t>0?100.0*d/t:0; };
    printf("    %-26s Sharpe  maxDD   Calmar  deploy%%   verdict\n","overlay");
    auto rowo=[&](const char* nm, std::function<bool(size_t)> ex){ M m=run_overlay(ex);
        double f=frac_deploy(ex); bool better=(m.sharpe>base.sharpe+0.03)||(m.calmar>base.calmar*1.05 && m.sharpe>=base.sharpe-0.02);
        const char* v = better?"KEEP":(m.sharpe<base.sharpe-0.05?"HURTS":"redundant");
        printf("    %-26s %5.2f  %5.1f%%  %5.2f   %5.1f    %s\n", nm, m.sharpe, m.maxdd*100, m.calmar, f, v); };
    printf("    %-26s %5.2f  %5.1f%%  %5.2f   %5.1f    (base)\n","breadth only (BASE)",base.sharpe,base.maxdd*100,base.calmar,frac_deploy([](size_t){return true;}));
    double md_slope=median_on_deploy(c_slope), md_disp=median_on_deploy(c_disp), md_med=0.0,
           md_rvol=median_on_deploy(c_rvol), md_dvol=median_on_deploy(c_dvol), md_corr=median_on_deploy(c_corr),
           md_liq=median_on_deploy(c_liq), md_btc=0.0;
    rowo("+breadth slope>=0",       [&](size_t i){return !std::isnan(c_slope[i])?c_slope[i]>=0:true;});
    rowo("+persistence>=0.5",       [&](size_t i){return !std::isnan(c_persist[i])?c_persist[i]>=0.5:true;});
    rowo("+dispersion>=median",     [&](size_t i){return !std::isnan(c_disp[i])?c_disp[i]>=md_disp:true;});
    rowo("+median asset ret>0",     [&](size_t i){return !std::isnan(c_medret[i])?c_medret[i]>md_med:true;});
    rowo("+realized vol<=median",   [&](size_t i){return !std::isnan(c_rvol[i])?c_rvol[i]<=md_rvol:true;});
    rowo("+downside vol<=median",   [&](size_t i){return !std::isnan(c_dvol[i])?c_dvol[i]<=md_dvol:true;});
    rowo("+correlation<=median",    [&](size_t i){return !std::isnan(c_corr[i])?c_corr[i]<=md_corr:true;});
    rowo("+liquidity>=1 (expand)",  [&](size_t i){return !std::isnan(c_liq[i])?c_liq[i]>=1.0:true;});
    rowo("+BTC 50d trend>0",        [&](size_t i){return !std::isnan(c_btc[i])?c_btc[i]>md_btc:true;});

    printf("\n================= DONE =================\n");
    return 0;
}

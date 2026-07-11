// ============================================================================
// entry_vs_retention_bt.cpp  —  PHASE 8D  ENTRY-vs-RETENTION  (research/analysis
// only — splits the ONE deploy gate into TWO scores; does NOT change any entry-
// engine logic, does NOT force-liquidate the validated fat-tail winners).
//
//   8B ([[ChimeraPhase8B]]): the edge is DEPLOYMENT-TIMING; the fat tail is
//   carried by RipRider/UpJump-family riders that ride their OWN wide exits.
//   8C ([[ChimeraPhase8C]]): the breadth-only continuous curve (B1 =
//   RegimeExposure::raw_curve) is near-optimal as a SINGLE deploy gate; a
//   co-equal 5-component blend washes breadth out. The conservative band is the
//   drawdown lever.
//
//   8D asks a genuinely different question. A SINGLE gate (B1) is SYMMETRIC:
//   the deployed fraction tracks the SAME breadth curve up AND down, so when the
//   tape dips it INSTANTLY cuts exposure — including cutting a fat-tail rider
//   mid-ride. 8D splits the regime decision into TWO scores:
//
//     ENTRY-deployment score  — controls NEW risk (adds). TIGHTER: adds are
//       FROZEN the moment breadth stops rising (slope gate), so new risk stops
//       EARLY when breadth weakens.
//     RETENTION score         — controls whether EXISTING exposure may continue.
//       LOOSER: held exposure is NOT force-cut on a regime dip; the excess above
//       what the tape currently supports bleeds off GRADUALLY (weak/marginal
//       exposure reduced FIRST, natural exit), and validated fat-tail winners
//       ride their OWN trailing exit — UNLESS a HARD defensive state (severe
//       breadth collapse) triggers an immediate flat.
//
//   Models tested (all on the SAME beta = EW-eligible-universe return `ewret`,
//   the market the retained book rides; entry engines untouched — the deploy
//   fraction is a multiplier that applies identically to the real book):
//
//     B1  single breadth deploy gate (live raw_curve) — enter AND exit on the
//         same score. THE BASELINE (8C's near-optimal single gate).
//     8D-A  asymmetric single exposure: tight slope-gated adds + loose bleed on
//           cuts + hard-defensive flat. Sweep entry-tightness x retention-bleed.
//     8D-B  two-sleeve: CORE (asymmetric as 8D-A, reduced-first) + RIDER sleeve
//           (fat-tail winner: deploys with the core, then rides its OWN trailing
//           exit, immune to regime dips except hard-defensive). Literal encoding
//           of "winners ride their own exits, weak reduced first."
//
//   KEY question: does entry-vs-retention PRESERVE the fat-tail upside (CAGR near
//   B1) while cutting drawdown / new-risk-into-a-falling-tape (better Calmar /
//   Ulcer / fewer false adds) vs the symmetric single gate?  Swept for a PLATEAU,
//   not one lucky cell. Extra fat-tail diagnostics: top-decile up-day capture,
//   post-dip rebound capture, new-risk-into-falling-tape.
//
//   Long-only spot, NO shorts, NO 200DMA (regime = smoothed BREADTH; BTC-trend
//   is ONE trailing-return component, never a 200DMA gate). All metrics EX-2022
//   (2022 bear SHOWN not gated). BACKTEST_TRUTH.
//
//   build: g++ -std=c++20 -O2 -I../include entry_vs_retention_bt.cpp -o entry_vs_retention_bt
//   run:   ./entry_vs_retention_bt      (from backtest/, needs ../data/xsec_seed)
// ============================================================================
#include "live/RegimeExposure.hpp"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

using namespace chimera;

// ---------------------------------------------------------------------------
// data  (identical universe + loader to exposure_mapping_bt.cpp / regime_decomp_bt.cpp)
// ---------------------------------------------------------------------------
static const std::vector<std::string> UNIVERSE = {
    "AAVE","ADA","ALGO","APE","AR","ATOM","AVAX","AXS","BAT","BCH","BNB","BTC",
    "CAKE","CHZ","COMP","CRV","DASH","DOGE","DOT","EGLD","ENJ","ETC","ETH","FET",
    "FIL","GALA","GRT","HBAR","ICP","INJ","KSM","LDO","LINK","LTC","MANA","NEAR",
    "NEO","ONE","QTUM","RUNE","SAND","SHIB","SNX","SOL","SUSHI","THETA","TRX",
    "UNI","VET","XLM","XRP","XTZ","ZEC","ZIL" };

static const int64_t X22lo=1640995200000LL, X22hi=1672531200000LL; // 2022 bear (ms)

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
            (void)h;(void)l;
            if(c>0) out[s].push_back({ts/86400000LL,o,h,l,c,c*v});
        }
        fclose(f);
    }
    return out;
}

struct Grid {
    std::vector<int64_t> days;
    std::map<int64_t,size_t> idx;
    std::vector<std::string> syms;
    std::vector<std::vector<double>> close, open, dvol;
    size_t N() const { return days.size(); }
    size_t S() const { return syms.size(); }
    double ret(size_t s, size_t i) const {
        if(i==0) return NAN; double a=close[s][i-1], b=close[s][i];
        return (!std::isnan(a)&&!std::isnan(b)&&a>0)? b/a-1.0 : NAN;
    }
    double tret(size_t s, size_t i, int lb) const {
        if((int)i<lb) return NAN; double a=close[s][i-lb], b=close[s][i];
        return (!std::isnan(a)&&!std::isnan(b)&&a>0)? b/a-1.0 : NAN;
    }
    bool listed(size_t s, size_t i, int hist=120) const {
        if((int)i<hist||i>=close[s].size()) return false;
        return !std::isnan(close[s][i]) && close[s][i]>0
            && !std::isnan(close[s][i-hist]) && close[s][i-hist]>0;
    }
};
static Grid build_grid(const std::map<std::string,std::vector<Row>>& data){
    Grid g; std::set<int64_t> ds;
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

static const int    LB=30;      // momentum / breadth lookback
static const int    SMOOTH=5;   // breadth smoothing
static const int    SLOPE_LB=10;// breadth slope lookback (matches 8B/8C slope component)
static const double BTHR=0.40;  // binary-gate breadth threshold (current model)

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
static std::vector<size_t> elig(const Grid& g, size_t i){
    std::vector<size_t> e; for(size_t s=0;s<g.S();++s) if(g.listed(s,i)) e.push_back(s); return e;
}

// ---------------------------------------------------------------------------
// metrics — SAME extended set as 8C (best != highest Sharpe)
// ---------------------------------------------------------------------------
struct EM {
    double total=0,cagr=0,sharpe=0,maxdd=0,calmar=0,ulcer=0;
    double avg_deploy=0, ret_on_deployed=0, turnover=0;
    int    dd_recover_days=0;
    int    n_adds=0, n_false_adds=0; double avg_fwd10_on_add=0, avg_capital_false=0;
    int    days=0;
};

static EM eval_series(const std::vector<int64_t>& day,
                      const std::vector<double>& r,
                      const std::vector<double>& depf,
                      const std::vector<double>& mkt_fwd10 /*aligned to day*/){
    EM m; size_t n=r.size(); m.days=(int)n; if(n==0) return m;
    double eq=1,peak=1,mean=0; std::vector<double> ddser; ddser.reserve(n);
    int under=0, worst_under=0;
    for(size_t i=0;i<n;++i){ double x=r[i]; mean+=x; eq*=(1+x);
        if(eq>=peak){ peak=eq; under=0; } else { ++under; worst_under=std::max(worst_under,under); }
        double dd=(peak-eq)/peak; ddser.push_back(dd); m.maxdd=std::max(m.maxdd,dd); }
    m.total=eq-1; mean/=n;
    double var=0; for(double x:r){double d=x-mean;var+=d*d;} var/=n;
    double sd=var>0?std::sqrt(var):0; m.sharpe=sd>0?mean/sd*std::sqrt(365.0):0;
    double yrs=n/365.0; m.cagr=yrs>0?std::pow(std::max(1e-9,eq),1.0/yrs)-1.0:0;
    m.calmar=m.maxdd>1e-6?m.cagr/m.maxdd:0;
    double us=0; for(double d:ddser) us+=d*d; m.ulcer=std::sqrt(us/n)*100.0;
    m.dd_recover_days=worst_under;
    double ad=0; for(double d:depf) ad+=d; m.avg_deploy=depf.empty()?0:ad/depf.size();
    m.ret_on_deployed=m.avg_deploy>1e-6?m.cagr/m.avg_deploy:0;
    for(size_t i=1;i<depf.size();++i) m.turnover+=std::fabs(depf[i]-depf[i-1]);
    // false-positive regime transition: an ADD = deploy_frac rises from <0.25 to
    // >=0.50; false if the fwd-10d MARKET return was negative.
    for(size_t i=1;i<depf.size();++i){
        if(depf[i-1]<0.25 && depf[i]>=0.50){
            m.n_adds++; double f=mkt_fwd10[i]; if(!std::isnan(f)) m.avg_fwd10_on_add+=f;
            if(!std::isnan(f)&&f<0){ m.n_false_adds++; m.avg_capital_false+=depf[i]; }
        }
    }
    if(m.n_adds>0) m.avg_fwd10_on_add/=m.n_adds;
    if(m.n_false_adds>0) m.avg_capital_false/=m.n_false_adds;
    return m;
}

static void print_hdr(){
    printf("  %-28s CAGR    maxDD  Calmar  Ulcer  avgDep  RoDC   turn   ddRec  FP-add  Sharpe\n","mapping");
}
static void print_row(const char* nm, const EM& m){
    printf("  %-28s %+6.0f%% %5.1f%% %6.2f %6.2f %5.1f%% %5.2f %6.1f %5dd  %d/%-3d %6.2f\n",
        nm, m.cagr*100, m.maxdd*100, m.calmar, m.ulcer, m.avg_deploy*100,
        m.ret_on_deployed, m.turnover, m.dd_recover_days, m.n_false_adds, m.n_adds, m.sharpe);
}

int main(int argc, char** argv){
    std::string dir=(argc>1)?argv[1]:"../data/xsec_seed";
    auto data=load_universe(dir); Grid g=build_grid(data);
    size_t N=g.N();
    printf("Loaded %zu/%zu symbols, %zu day axis (%s)\n", data.size(), UNIVERSE.size(), N, dir.c_str());
    printf("PHASE 8D ENTRY-vs-RETENTION. Long-only spot, NO 200DMA, NO shorts; all metrics\n");
    printf("EX-2022 (bear shown, not gated). Baseline = B1 single breadth deploy gate (8C).\n\n");

    // ---- breadth (raw+smoothed) + slope ----
    std::vector<double> braw(N), bsm(N), slope10(N,0.0);
    for(size_t i=0;i<N;++i) braw[i]=breadth_raw(g,i);
    for(size_t i=0;i<N;++i) bsm[i]=breadth_sm(braw,i);
    for(size_t i=0;i<N;++i) if(i>=(size_t)SLOPE_LB) slope10[i]=bsm[i]-bsm[i-SLOPE_LB];

    // ---- EW-eligible market return (the book's beta) + fwd-10d market ----
    std::vector<double> ewret(N,NAN);
    for(size_t i=1;i<N;++i){ auto e=elig(g,i-1); double r=0;int n=0;
        for(size_t s:e){ double x=g.ret(s,i); if(!std::isnan(x)){r+=x;++n;} } if(n>0) ewret[i]=r/n; }
    std::vector<double> fwd10(N,NAN);
    for(size_t i=0;i<N;++i){ double p=1; bool ok=true; int cnt=0;
        for(size_t j=i+1;j<N&&cnt<10;++j,++cnt){ if(std::isnan(ewret[j])){ok=false;break;} p*=(1+ewret[j]); }
        if(ok&&cnt>0) fwd10[i]=p-1; }

    auto in22=[&](size_t i){ int64_t t=g.days[i]*86400000LL; return t>=X22lo&&t<X22hi; };

    // ---- shared runner: given a deploy-frac provider, build the EX-2022 series ----
    auto run=[&](std::function<double(size_t)> depf_at)->EM{
        std::vector<int64_t> day; std::vector<double> r, depf, f10;
        for(size_t i=1;i<N;++i){ if(in22(i)) continue;
            double d=depf_at(i-1); if(std::isnan(d)) d=0;
            double mr=ewret[i]; if(std::isnan(mr)) mr=0;
            day.push_back(g.days[i]); r.push_back(d*mr); depf.push_back(d); f10.push_back(fwd10[i-1]);
        }
        return eval_series(day,r,depf,f10);
    };

    // ============================================================
    // BASELINE — B1 single breadth deploy gate (8C's near-optimal single score)
    //   SYMMETRIC: deploy fraction tracks raw_curve(bsm) up AND down instantly.
    // ============================================================
    std::vector<double> depB1(N,0.0);
    for(size_t i=0;i<N;++i) depB1[i]=RegimeExposure::raw_curve(bsm[i]);
    EM b1=run([&](size_t i){ return depB1[i]; });

    // ============================================================
    // 8D-A — ENTRY-vs-RETENTION, single asymmetric exposure.
    //   ENTRY (adds): TIGHT. Adds allowed only when breadth is high enough AND
    //     RISING (slope >= slope_gate). New risk FROZEN the moment breadth stops
    //     rising — stops new risk early when the tape weakens.
    //   RETENTION (holds/cuts): LOOSE. Existing exposure is NOT force-cut on a
    //     regime dip; the EXCESS above what breadth now supports (retain_target =
    //     raw_curve) bleeds off gradually (weak/marginal exposure reduced FIRST,
    //     natural exit) at rate `bleed`; below the retain_floor the natural exit
    //     accelerates.
    //   HARD DEFENSIVE: breadth < hard_floor => immediate flat (severe alarm).
    //   Nests B1 exactly at (slope_gate=-inf, entry_bar=0, bleed=1).
    // ============================================================
    struct ERP { double slope_gate; double bleed; double entry_bar; double retain_floor; double hard_floor; };
    auto alloc_A=[&](ERP p)->std::vector<double>{
        std::vector<double> dep(N,0.0); double D=0.0;
        for(size_t i=0;i<N;++i){
            double b=bsm[i], sl=slope10[i];
            double cap=RegimeExposure::raw_curve(b);   // what a full deploy would be
            double retain=cap;                          // level the tape now supports
            if(b<p.hard_floor){ D=0.0; }                // severe -> flat
            else{
                bool can_add = (b>=p.entry_bar && sl>=p.slope_gate);
                if(can_add && cap>D) D=cap;             // validated add (tight gate)
                if(D>retain){                            // weak-first natural bleed
                    double rate = (b>=p.retain_floor)? p.bleed : std::min(1.0,p.bleed*3.0);
                    D = retain + (1.0-rate)*(D-retain);
                }
                // if !can_add and D<=retain: HOLD D (no forced cut) -> winners ride
            }
            dep[i]=D;
        }
        return dep;
    };
    auto run_dep=[&](const std::vector<double>& dep)->EM{
        return run([&](size_t i){ return dep[i]; });
    };

    // ---- continuity sanity: nesting recovers B1 ----
    EM nest=run_dep(alloc_A({-1e9, 1.0, 0.0, 0.0, 0.0}));

    printf("=== BASELINE + NESTING SANITY ===\n"); print_hdr();
    print_row("B1 single breadth gate", b1);
    print_row("8D-A nested->B1 (check)", nest);
    printf("    (8D-A at slope_gate=-inf,entry_bar=0,bleed=1 must reproduce B1 -> continuity)\n\n");

    // ---- PLATEAU SWEEP: entry-tightness (slope_gate) x retention-looseness (bleed) ----
    std::vector<double> gates = {-1e9, -0.03, -0.01, 0.00, 0.02, 0.04}; // higher = tighter adds
    std::vector<const char*> gate_nm = {"off","-.03","-.01"," .00"," .02"," .04"};
    std::vector<double> bleeds= {0.06, 0.12, 0.25, 0.50, 1.00};        // lower = looser retention
    const double ENTRY_BAR=0.35, RETAIN_FLOOR=0.25, HARD=0.15;

    printf("=== 8D-A PLATEAU: CALMAR grid (entry slope-gate down, retention bleed across) ===\n");
    printf("   (higher slope-gate = tighter/earlier add-freeze; lower bleed = looser/longer ride)\n");
    printf("   B1 baseline Calmar = %.2f\n", b1.calmar);
    printf("   slope\\bleed");
    for(double bl:bleeds) printf("   %.2f", bl);
    printf("\n");
    for(size_t gi=0;gi<gates.size();++gi){
        printf("     %5s   ", gate_nm[gi]);
        for(double bl:bleeds){
            EM m=run_dep(alloc_A({gates[gi],bl,ENTRY_BAR,RETAIN_FLOOR,HARD}));
            printf("  %5.2f", m.calmar);
        }
        printf("\n");
    }
    printf("\n=== 8D-A PLATEAU: maxDD%% grid (same axes) ===\n");
    printf("   B1 baseline maxDD = %.1f%%\n", b1.maxdd*100);
    printf("   slope\\bleed");
    for(double bl:bleeds) printf("   %.2f", bl);
    printf("\n");
    for(size_t gi=0;gi<gates.size();++gi){
        printf("     %5s   ", gate_nm[gi]);
        for(double bl:bleeds){
            EM m=run_dep(alloc_A({gates[gi],bl,ENTRY_BAR,RETAIN_FLOOR,HARD}));
            printf("  %5.1f", m.maxdd*100);
        }
        printf("\n");
    }
    printf("\n=== 8D-A PLATEAU: CAGR%% grid (fat-tail upside must survive) ===\n");
    printf("   B1 baseline CAGR = %+.0f%%\n", b1.cagr*100);
    printf("   slope\\bleed");
    for(double bl:bleeds) printf("   %.2f", bl);
    printf("\n");
    for(size_t gi=0;gi<gates.size();++gi){
        printf("     %5s   ", gate_nm[gi]);
        for(double bl:bleeds){
            EM m=run_dep(alloc_A({gates[gi],bl,ENTRY_BAR,RETAIN_FLOOR,HARD}));
            printf("  %+5.0f", m.cagr*100);
        }
        printf("\n");
    }

    // ---- full-metric rows for representative cells + a picked plateau centre ----
    printf("\n=== 8D-A representative cells (full metrics) vs B1 ===\n"); print_hdr();
    print_row("B1 single gate (baseline)", b1);
    struct Cell{const char* nm; ERP p;};
    std::vector<Cell> cells = {
        {"tight-add loose-ride",  {0.00, 0.12, ENTRY_BAR, RETAIN_FLOOR, HARD}},
        {"tight-add med-ride",    {0.00, 0.25, ENTRY_BAR, RETAIN_FLOOR, HARD}},
        {"v.tight-add loose-ride",{0.02, 0.12, ENTRY_BAR, RETAIN_FLOOR, HARD}},
        {"mild-add loose-ride",   {-0.01,0.12, ENTRY_BAR, RETAIN_FLOOR, HARD}},
        {"tight-add v.loose-ride",{0.00, 0.06, ENTRY_BAR, RETAIN_FLOOR, HARD}},
        {"add-freeze-only (bleed1)",{0.00,1.00, ENTRY_BAR, RETAIN_FLOOR, HARD}},
    };
    std::vector<double> cell_deps_for_diag; // keep the plateau centre for diagnostics
    for(auto& c:cells) print_row(c.nm, run_dep(alloc_A(c.p)));

    // pick a plateau-centre config for downstream diagnostics + 8D-B
    ERP PLATEAU{0.00, 0.12, ENTRY_BAR, RETAIN_FLOOR, HARD};
    std::vector<double> depA = alloc_A(PLATEAU);

    // ============================================================
    // 8D-B — two-sleeve. CORE (asymmetric, reduced-first as 8D-A) up to CORE_MAX
    //   of the raw_curve deployment; RIDER sleeve up to RIDER_MAX that deploys
    //   WITH the core when entry is permitted, then rides its OWN trailing exit
    //   (drawdown from its running peak > rider_trail) — it is NOT reduced on a
    //   regime dip (only a hard-defensive breach or its own trail flattens it).
    //   Literal encoding of "fat-tail winners ride their own validated exits;
    //   weak/marginal (the core) reduced first."
    // ============================================================
    auto alloc_B=[&](ERP p, double core_max, double rider_max, double rider_trail)->std::vector<double>{
        std::vector<double> dep(N,0.0);
        double Dc=0.0;                 // core exposure (0..core_max*cap)
        double Dr=0.0;                 // rider exposure (0 or rider_max*cap_at_entry)
        double rider_eq=1.0, rider_peak=1.0; bool rider_on=false;
        for(size_t i=0;i<N;++i){
            double b=bsm[i], sl=slope10[i];
            double cap=RegimeExposure::raw_curve(b);
            bool can_add = (b>=p.entry_bar && sl>=p.slope_gate);
            if(b<p.hard_floor){ Dc=0.0; Dr=0.0; rider_on=false; }
            else{
                // ---- CORE: asymmetric entry/retention, reduced FIRST ----
                double core_cap=core_max*cap, core_retain=core_cap;
                if(can_add && core_cap>Dc) Dc=core_cap;
                if(Dc>core_retain){
                    double rate=(b>=p.retain_floor)? p.bleed : std::min(1.0,p.bleed*3.0);
                    Dc = core_retain + (1.0-rate)*(Dc-core_retain);
                }
                // ---- RIDER: deploy with the core, then ride its OWN exit ----
                if(!rider_on){
                    if(can_add && cap>0){ rider_on=true; Dr=rider_max*cap; rider_eq=1.0; rider_peak=1.0; }
                } else {
                    // rider rides regime dips; exits ONLY on its own trailing stop.
                    if(i+1<N){ double mr=ewret[i]; if(!std::isnan(mr)) rider_eq*=(1+mr); }
                    rider_peak=std::max(rider_peak,rider_eq);
                    double ddown=(rider_peak-rider_eq)/rider_peak;
                    if(ddown>rider_trail){ rider_on=false; Dr=0.0; }
                    // note: rider size is NOT re-cut by regime; it holds until its trail.
                }
            }
            dep[i]=std::clamp(Dc+Dr,0.0,0.95);
        }
        return dep;
    };

    printf("\n=== 8D-B two-sleeve (core reduced-first + fat-tail RIDER on own trailing exit) ===\n");
    print_hdr();
    print_row("B1 single gate (baseline)", b1);
    print_row("8D-A plateau centre",       run_dep(depA));
    struct BCell{const char* nm; double cmax,rmax,trail;};
    std::vector<BCell> bcells = {
        {"core.6/rider.35 trail20", 0.60,0.35,0.20},
        {"core.6/rider.35 trail30", 0.60,0.35,0.30},
        {"core.5/rider.45 trail25", 0.50,0.45,0.25},
        {"core.7/rider.25 trail25", 0.70,0.25,0.25},
        {"core.6/rider.35 trail15", 0.60,0.35,0.15},
    };
    for(auto& c:bcells) print_row(c.nm, run_dep(alloc_B(PLATEAU,c.cmax,c.rmax,c.trail)));

    // ============================================================
    // FAT-TAIL / NEW-RISK DIAGNOSTICS — the KEY question, decomposed.
    //   Compare B1 vs 8D-A(plateau) vs 8D-B(centre) on:
    //   (1) top-decile UP-day capture  = avg exposure on the biggest up-days
    //       (higher = kept more of the fat right tail).
    //   (2) post-dip REBOUND capture   = avg exposure on days where breadth was
    //       weakening (slope<0) yet the fwd-10d market rebounded > +10%
    //       (higher = retention let winners ride the rebound instead of cutting).
    //   (3) new-risk-into-a-FALLING-tape = avg exposure ADDED (positive d-deploy)
    //       on days where slope<0 AND fwd-10d market < 0 (lower = tighter entry
    //       stopped new risk early).
    // ============================================================
    std::vector<double> depBcentre = alloc_B(PLATEAU,0.60,0.35,0.30);
    auto diag=[&](const char* nm, const std::vector<double>& dep){
        // build ex-2022 aligned vectors of (exposure applied at i-1, ewret[i], slope, fwd10)
        struct Rec{ double d; double mr; double sl; double f10; };
        std::vector<Rec> v;
        for(size_t i=1;i<N;++i){ if(in22(i)) continue;
            double d=dep[i-1]; if(std::isnan(d)) d=0;
            double mr=ewret[i]; if(std::isnan(mr)) continue;
            v.push_back({d,mr,slope10[i-1],fwd10[i-1]});
        }
        // (1) top-decile up-day capture
        std::vector<double> mrs; for(auto&r:v) mrs.push_back(r.mr);
        std::sort(mrs.begin(),mrs.end());
        double thr = mrs.empty()?0.0:mrs[(size_t)(0.90*mrs.size())];
        double capexp=0; int capn=0;
        for(auto&r:v) if(r.mr>=thr){ capexp+=r.d; ++capn; }
        double topcap = capn>0?capexp/capn:0;
        // (2) post-dip rebound capture
        double rbexp=0; int rbn=0;
        for(auto&r:v) if(r.sl<0 && !std::isnan(r.f10) && r.f10>0.10){ rbexp+=r.d; ++rbn; }
        double rebcap = rbn>0?rbexp/rbn:0;
        // (3) new-risk into a falling tape: positive d-deploy on slope<0 & fwd10<0
        double addfall=0; int addn=0;
        for(size_t k=1;k<v.size();++k){ double dd=v[k].d-v[k-1].d;
            if(dd>0 && v[k].sl<0 && !std::isnan(v[k].f10) && v[k].f10<0){ addfall+=dd; ++addn; } }
        printf("  %-24s top-decile-up cap %.2f | post-dip rebound cap %.2f | new-risk-into-fall %.3f (%d days)\n",
               nm, topcap, rebcap, addfall, addn);
    };
    // ---- ENTRY-TIGHTNESS robustness: the ONLY beneficial fragment is the tight
    //      ENTRY (add-freeze + entry_bar floor) at bleed=1 (tight retention). Is
    //      that ~0.18-Calmar gain stable across the entry_bar floor, or a lucky
    //      single floor? Sweep entry_bar with slope_gate=.00, bleed=1.0. ----
    printf("\n=== ENTRY-TIGHTNESS robustness (slope_gate=.00, bleed=1.0; is the gain stable?) ===\n"); print_hdr();
    print_row("B1 single gate (baseline)", b1);
    for(double eb : {0.25,0.30,0.35,0.40,0.45}){
        char nm[48]; snprintf(nm,sizeof nm,"tight-entry bar=%.2f",eb);
        print_row(nm, run_dep(alloc_A({0.00,1.00,eb,RETAIN_FLOOR,HARD})));
    }

    printf("\n=== FAT-TAIL / NEW-RISK DIAGNOSTICS (ex-2022) ===\n");
    printf("  (1) top-decile up-day capture: higher = kept more of the fat right tail\n");
    printf("  (2) post-dip rebound capture:  higher = retention let winners ride the rebound\n");
    printf("  (3) new-risk-into-falling-tape: lower = tight entry stopped new risk early\n");
    diag("B1 single gate", depB1);
    diag("8D-A plateau centre", depA);
    diag("8D-B centre (core+rider)", depBcentre);

    printf("\n================= DONE =================\n");
    return 0;
}

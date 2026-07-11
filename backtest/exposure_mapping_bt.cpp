// ============================================================================
// exposure_mapping_bt.cpp  —  PHASE 8C  EXPOSURE MAPPING  (research/analysis
// only — optimizes the DEPLOYMENT AMOUNT on top of the SAME beta; does NOT
// change any entry-engine logic).
//
//   Per Phase-8B ([[ChimeraPhase8B]]) the edge is DEPLOYMENT-TIMING, and vol
//   belongs in SIZING not the gate.  8C therefore builds:
//
//   (1) DEPLOYMENT SCORE  s in [0,1] = f(breadth level, breadth slope,
//       correlation state, BTC-trend 50d, median asset return) — the FIVE
//       KEEP components from the 8B ablation, each turned into a [0,1]
//       sub-score by its TRAILING empirical percentile (expanding window,
//       data <= i-1 only — NO look-ahead), correlation INVERTED (crowding
//       cuts).  Equal-weight blend (each contribution understandable).
//
//   (2) SCORE -> DEPLOY%  mapping via conservative / balanced / aggressive
//       band tables (roadmap balanced: 0-.2->0 / .2-.4->20 / .4-.6->50 /
//       .6-.8->75 / .8-1->95), with ASYMMETRIC HYSTERESIS — add risk only
//       when the score clears the next band edge by +0.05; cut immediately
//       on a drop.  Harder to add than to remove.
//
//   (3) NOTIONAL SIZING on the deployed capital by realized-vol / downside-
//       vol / liquidity — these SIZE the deployed amount, they do NOT gate
//       (8B showed they hurt as an on/off filter but belong in sizing).
//
//   Baselines for the tradeoff table:
//       B0  binary breadth gate  (deploy in {0,1} at breadth>=0.40 — the
//           "current simple regime", = 8B's L1)
//       B1  breadth-only continuous curve (live RegimeExposure::raw_curve)
//
//   Underlying book each day = EW-eligible-universe return (the market the
//   retained book rides — 8B's ewret).  Portfolio return[i] =
//   deploy_frac[i-1] * size_frac[i-1] * market_ret[i]; the rest sits in cash
//   (0%).  This isolates the DEPLOYMENT-AMOUNT question exactly (entry
//   engines untouched — the multiplier applies identically to the real book).
//
//   Metrics (best != highest Sharpe): CAGR, maxDD, Calmar, Ulcer index, avg
//   deployed capital, return-on-deployed-capital, turnover, drawdown-recovery
//   time, and FALSE-POSITIVE regime-transition behaviour.
//
//   Long-only spot, NO shorts, NO 200DMA (regime = smoothed BREADTH; BTC-trend
//   is ONE trailing-return component, never a 200DMA gate).  All metrics
//   EX-2022 (2022 bear SHOWN not gated).  BACKTEST_TRUTH.
//
//   build: g++ -std=c++20 -O2 -I../include exposure_mapping_bt.cpp -o exposure_mapping_bt
//   run:   ./exposure_mapping_bt        (from backtest/, needs ../data/xsec_seed)
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
// data  (identical universe + loader to regime_decomp_bt.cpp)
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
// metrics — extended set for the 8C tradeoff table
// ---------------------------------------------------------------------------
struct EM {
    double total=0,cagr=0,sharpe=0,maxdd=0,calmar=0,ulcer=0;
    double avg_deploy=0, ret_on_deployed=0, turnover=0;
    int    dd_recover_days=0;
    // false-positive regime transitions
    int    n_adds=0, n_false_adds=0; double avg_fwd10_on_add=0, avg_capital_false=0;
    int    days=0;
};

// series = (day, portfolio_return, deploy_frac_applied)
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
    // turnover = sum |d deploy_frac|
    for(size_t i=1;i<depf.size();++i) m.turnover+=std::fabs(depf[i]-depf[i-1]);
    // false-positive regime transitions: an ADD = deploy_frac rises from <0.25
    // (recent low) to >=0.50; false if the fwd-10d MARKET return was negative.
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
    printf("  %-26s CAGR    maxDD  Calmar  Ulcer  avgDep  RoDC   turn   ddRec  FP-add\n","mapping");
}
static void print_row(const char* nm, const EM& m){
    printf("  %-26s %+6.0f%% %5.1f%% %6.2f %6.2f %5.1f%% %5.2f %6.1f %5dd  %d/%d\n",
        nm, m.cagr*100, m.maxdd*100, m.calmar, m.ulcer, m.avg_deploy*100,
        m.ret_on_deployed, m.turnover, m.dd_recover_days, m.n_false_adds, m.n_adds);
}

// ---------------------------------------------------------------------------
// trailing empirical percentile (expanding window, uses ONLY x[0..i-1] -> the
// value x[i]'s rank among strictly-prior observations; NO look-ahead)
// ---------------------------------------------------------------------------
static std::vector<double> trailing_pct(const std::vector<double>& x, int warm=120){
    size_t N=x.size(); std::vector<double> p(N,NAN);
    std::vector<double> hist; hist.reserve(N);
    for(size_t i=0;i<N;++i){
        double v=x[i];
        if(!std::isnan(v) && (int)hist.size()>=warm){
            // percentile = share of prior history below v
            int below=0,cnt=0; for(double h:hist){ if(std::isnan(h))continue; ++cnt; if(h<v)++below; }
            if(cnt>0) p[i]=(double)below/cnt;
        }
        if(!std::isnan(v)) hist.push_back(v);
    }
    return p;
}

// ---------------------------------------------------------------------------
// band mapping with ASYMMETRIC HYSTERESIS
//   edges[] ascending in [0,1]; deploy[] the deploy% for band b = [edges[b],edges[b+1]).
//   natural band n = largest b with score>=edges[b].
//   to STEP UP a band, score must clear the new band's lower edge by +HYST.
//   to STEP DOWN, drop immediately (score < current band lower edge).
// ---------------------------------------------------------------------------
struct Bands { std::vector<double> edge; std::vector<double> dep; };
static const double HYST=0.05;
static int natural_band(const Bands& b, double s){
    int k=0; for(int i=0;i<(int)b.edge.size();++i) if(s>=b.edge[i]) k=i; return k;
}
// returns deploy fraction given prior band index (in/out), applying hysteresis
static double band_deploy(const Bands& b, double s, int& cur){
    int nat=natural_band(b,s);
    if(nat>cur){
        // only step up as far as hysteresis permits (score must clear edge+HYST)
        int step=cur;
        for(int k=cur+1;k<=nat;++k){ if(s>=b.edge[k]+HYST) step=k; else break; }
        cur=step;
    } else if(nat<cur){
        cur=nat; // cut immediately
    }
    return b.dep[cur];
}

int main(int argc, char** argv){
    std::string dir=(argc>1)?argv[1]:"../data/xsec_seed";
    auto data=load_universe(dir); Grid g=build_grid(data);
    size_t N=g.N();
    printf("Loaded %zu/%zu symbols, %zu day axis (%s)\n", data.size(), UNIVERSE.size(), N, dir.c_str());
    printf("Long-only spot, NO 200DMA, NO shorts; all metrics EX-2022 (bear shown, not gated).\n\n");

    // ---- breadth (raw+smoothed) ----
    std::vector<double> braw(N), bsm(N);
    for(size_t i=0;i<N;++i) braw[i]=breadth_raw(g,i);
    for(size_t i=0;i<N;++i) bsm[i]=breadth_sm(braw,i);

    // ---- EW-eligible market return (the book's beta) + fwd-10d market ----
    std::vector<double> ewret(N,NAN);
    for(size_t i=1;i<N;++i){ auto e=elig(g,i-1); double r=0;int n=0;
        for(size_t s:e){ double x=g.ret(s,i); if(!std::isnan(x)){r+=x;++n;} } if(n>0) ewret[i]=r/n; }
    std::vector<double> fwd10(N,NAN);
    for(size_t i=0;i<N;++i){ double p=1; bool ok=true; int cnt=0;
        for(size_t j=i+1;j<N&&cnt<10;++j,++cnt){ if(std::isnan(ewret[j])){ok=false;break;} p*=(1+ewret[j]); }
        if(ok&&cnt>0) fwd10[i]=p-1; }

    // ---- the FIVE KEEP components (8B ablation) ----
    std::vector<double> c_breadth(N,NAN), c_slope(N,NAN), c_medret(N,NAN),
                        c_corr(N,NAN), c_btc(N,NAN);
    // sizing inputs
    std::vector<double> c_rvol(N,NAN), c_dvol(N,NAN), c_liq(N,NAN);
    size_t btc_idx=SIZE_MAX; for(size_t s=0;s<g.S();++s) if(g.syms[s]=="BTC") btc_idx=s;
    for(size_t i=0;i<N;++i){
        c_breadth[i]=bsm[i];
        if(i>=10) c_slope[i]=bsm[i]-bsm[i-10];
        { auto e=elig(g,i); std::vector<double> rs;
          for(size_t s:e){ double tr=g.tret(s,i,LB); if(!std::isnan(tr)) rs.push_back(tr); }
          if(rs.size()>=2){ std::sort(rs.begin(),rs.end()); c_medret[i]=rs[rs.size()/2]; } }
        if(i>=31){ std::vector<double> r; for(size_t j=i-30;j<i;++j) if(!std::isnan(ewret[j])) r.push_back(ewret[j]);
          if(r.size()>=18){ double m=0; for(double x:r)m+=x; m/=r.size();
            double v=0,dn=0;int nd=0; for(double x:r){ v+=(x-m)*(x-m); if(x<0){dn+=x*x;++nd;} }
            c_rvol[i]=std::sqrt(v/r.size()); c_dvol[i]=nd>0?std::sqrt(dn/nd):0.0; } }
        if(i>=31){ auto e=elig(g,i); if(e.size()>=3){ if(e.size()>30) e.resize(30);
            double sum=0;int cnt=0;
            for(size_t a=0;a<e.size();++a) for(size_t b=a+1;b<e.size();++b){
                std::vector<double> ra,rb; for(size_t j=i-30;j<i;++j){ double xa=g.ret(e[a],j),xb=g.ret(e[b],j);
                    if(!std::isnan(xa)&&!std::isnan(xb)){ ra.push_back(xa); rb.push_back(xb);} }
                if(ra.size()<15) continue; double ma=0,mb=0; for(size_t k=0;k<ra.size();++k){ma+=ra[k];mb+=rb[k];} ma/=ra.size();mb/=rb.size();
                double sab=0,saa=0,sbb=0; for(size_t k=0;k<ra.size();++k){double da=ra[k]-ma,db=rb[k]-mb;sab+=da*db;saa+=da*da;sbb+=db*db;}
                if(saa>0&&sbb>0){ sum+=sab/std::sqrt(saa*sbb); ++cnt; } }
            if(cnt>0) c_corr[i]=sum/cnt; } }
        if(i>=91){ auto e=elig(g,i); double now=0,base=0;int n1=0,n2=0;
            for(size_t s:e){ double d=g.dvol[s][i]; if(!std::isnan(d)&&d>0){now+=d;++n1;}
                double bb=0;int bc=0; for(size_t j=i-90;j<i;++j){ double dd=g.dvol[s][j]; if(!std::isnan(dd)&&dd>0){bb+=dd;++bc;} }
                if(bc>0){ base+=bb/bc; ++n2; } }
            if(n1>0&&n2>0&&base>0) c_liq[i]=(now/n1)/(base/n2); }
        if(btc_idx!=SIZE_MAX){ double tr=g.tret(btc_idx,i,50); if(!std::isnan(tr)) c_btc[i]=tr; }
    }

    // ---- turn each KEEP component into a trailing-percentile sub-score [0,1] ----
    // correlation INVERTED (high crowding -> low score): score on (-corr).
    std::vector<double> ncorr(N,NAN); for(size_t i=0;i<N;++i) if(!std::isnan(c_corr[i])) ncorr[i]=-c_corr[i];
    auto p_breadth=trailing_pct(c_breadth);
    auto p_slope  =trailing_pct(c_slope);
    auto p_medret =trailing_pct(c_medret);
    auto p_corr   =trailing_pct(ncorr);
    auto p_btc    =trailing_pct(c_btc);

    // ---- composite DEPLOYMENT SCORE (equal-weight blend of available subs) ----
    std::vector<double> score(N,NAN);
    for(size_t i=0;i<N;++i){
        double sum=0;int k=0;
        for(double* pp : {&p_breadth[i],&p_slope[i],&p_medret[i],&p_corr[i],&p_btc[i]})
            if(!std::isnan(*pp)){ sum+=*pp; ++k; }
        if(k>=3) score[i]=sum/k;   // need most components warm
    }

    // ---- sizing factors on the DEPLOYED capital (trailing-percentile of risk;
    //      LOW risk -> full size, HIGH risk -> reduced size; floor 0.35 so it
    //      NEVER gates the up-moves off — it only scales) ----
    auto p_rvol=trailing_pct(c_rvol);   // high rvol -> high pct -> smaller size
    auto p_dvol=trailing_pct(c_dvol);
    auto sizefac=[&](const std::vector<double>& pr, size_t i)->double{
        if(std::isnan(pr[i])) return 1.0; return std::clamp(1.0 - 0.65*pr[i], 0.35, 1.0);
    };
    // liquidity sizing: contract size when liq<1 (thin tape), full when >=1
    auto sizeliq=[&](size_t i)->double{
        if(std::isnan(c_liq[i])) return 1.0; return std::clamp(c_liq[i], 0.5, 1.0);
    };

    // ---- band tables ----
    Bands balanced{   {0.0,0.20,0.40,0.60,0.80}, {0.00,0.20,0.50,0.75,0.95} };
    Bands conservative{{0.0,0.30,0.50,0.70,0.85}, {0.00,0.15,0.35,0.60,0.85} };
    Bands aggressive{ {0.0,0.15,0.30,0.50,0.70}, {0.00,0.35,0.65,0.85,0.95} };

    // ---- runner: given a deploy-frac provider, build portfolio series EX-2022 ----
    auto in22=[&](size_t i){ int64_t t=g.days[i]*86400000LL; return t>=X22lo&&t<X22hi; };
    auto run=[&](std::function<double(size_t)> depf_at)->EM{
        std::vector<int64_t> day; std::vector<double> r, depf, f10;
        for(size_t i=1;i<N;++i){ if(in22(i)) continue;
            double d=depf_at(i-1); if(std::isnan(d)) d=0;
            double mr=ewret[i]; if(std::isnan(mr)) mr=0;
            day.push_back(g.days[i]); r.push_back(d*mr); depf.push_back(d); f10.push_back(fwd10[i-1]);
        }
        return eval_series(day,r,depf,f10);
    };

    // baselines
    EM b0=run([&](size_t i){ return (bsm[i]>=BTHR)?1.0:0.0; });          // binary gate (L1)
    EM b1=run([&](size_t i){ return RegimeExposure::raw_curve(bsm[i]); });// breadth-only curve
    // B2: SAME continuous curve, but on the 5-component composite SCORE instead of
    // breadth alone — isolates "do the extra components help?" from "does banding hurt?"
    EM b2=run([&](size_t i){ return std::isnan(score[i])?0.0:RegimeExposure::raw_curve(score[i]); });

    // score distribution diagnostic (why banding centres on mid-deployment)
    { std::vector<double> sv; for(size_t i=0;i<N;++i) if(!std::isnan(score[i])&&!in22(i)) sv.push_back(score[i]);
      std::sort(sv.begin(),sv.end());
      auto q=[&](double p){ return sv.empty()?0.0:sv[std::min(sv.size()-1,(size_t)(p*sv.size()))]; };
      int lo=0,mid=0,hi=0; for(double v:sv){ if(v<0.2)++lo; else if(v>0.8)++hi; else ++mid; }
      printf("SCORE distribution (ex-2022): p10=%.2f p50=%.2f p90=%.2f  |  <0.2: %.0f%%  0.2-0.8: %.0f%%  >0.8: %.0f%%\n",
             q(0.1),q(0.5),q(0.9), 100.0*lo/sv.size(),100.0*mid/sv.size(),100.0*hi/sv.size());
      printf("  (an equal-weight blend of 5 uniform percentiles central-limits toward 0.5 -> the extreme\n");
      printf("   deploy bands rarely trigger; a CONTINUOUS curve on the score reads it without that dead-zone.)\n\n");
    }

    // mapping runner WITH asymmetric hysteresis on the score->band
    auto run_map=[&](const Bands& B, std::function<double(size_t)> sizer)->EM{
        int cur=0; std::vector<double> dep(N,NAN);
        for(size_t i=0;i<N;++i){ if(std::isnan(score[i])){ dep[i]=(i>0?dep[i-1]:0.0); continue; }
            double d=band_deploy(B,score[i],cur); dep[i]=d; }
        return run([&](size_t i){ double d=std::isnan(dep[i])?0.0:dep[i]; return d*sizer(i); });
    };
    auto nosize=[&](size_t){ return 1.0; };

    // =====================================================================
    printf("############################################################\n");
    printf("### DEPLOYMENT SCORE = mean trailing-percentile of the 5 KEEP\n");
    printf("### components (breadth level, breadth slope, correlation[inv],\n");
    printf("### BTC-trend 50d, median asset return). NO look-ahead.\n");
    printf("############################################################\n\n");
    printf("Metric legend: RoDC=return-on-deployed-capital (CAGR/avgDeploy),\n");
    printf("  turn=sum|d deploy|, ddRec=longest underwater stretch (days),\n");
    printf("  FP-add=false-positive adds / total adds (add=frac<.25 -> >=.50,\n");
    printf("  false if fwd-10d market return <0). Sharpe reported separately.\n\n");

    printf("=== BASELINES ===\n"); print_hdr();
    print_row("B0 binary breadth gate", b0);
    print_row("B1 breadth-only curve",  b1);
    print_row("B2 curve on 5-comp score", b2);
    printf("    (Sharpe: B0=%.2f  B1=%.2f  B2=%.2f)  [ddRec is splice-inflated: dropping 2022\n", b0.sharpe, b1.sharpe, b2.sharpe);
    printf("     leaves one 2021-peak->2023-recovery underwater span for ALL series -> non-discriminating]\n\n");

    printf("=== 8C SCORE->DEPLOY MAPPINGS (5-component score, hysteresis, NO sizing) ===\n"); print_hdr();
    EM mcon=run_map(conservative,nosize), mbal=run_map(balanced,nosize), magg=run_map(aggressive,nosize);
    print_row("conservative", mcon);
    print_row("balanced",     mbal);
    print_row("aggressive",   magg);
    printf("    (Sharpe: cons=%.2f  bal=%.2f  aggr=%.2f)\n\n", mcon.sharpe,mbal.sharpe,magg.sharpe);

    printf("=== + NOTIONAL SIZING on deployed capital (balanced band) ===\n"); print_hdr();
    EM sb_r =run_map(balanced,[&](size_t i){return sizefac(p_rvol,i);});
    EM sb_d =run_map(balanced,[&](size_t i){return sizefac(p_dvol,i);});
    EM sb_l =run_map(balanced,[&](size_t i){return sizeliq(i);});
    EM sb_rl=run_map(balanced,[&](size_t i){return sizefac(p_rvol,i)*sizeliq(i);});
    print_row("balanced (no sizing)",     mbal);
    print_row("bal + realized-vol size",  sb_r);
    print_row("bal + downside-vol size",  sb_d);
    print_row("bal + liquidity size",     sb_l);
    print_row("bal + rvol*liq size",      sb_rl);
    printf("    (Sharpe: base=%.2f rvol=%.2f dvol=%.2f liq=%.2f rvol*liq=%.2f)\n",
           mbal.sharpe,sb_r.sharpe,sb_d.sharpe,sb_l.sharpe,sb_rl.sharpe);

    // BREADTH-SPINE variant: keep breadth's curve as the deployment SPINE (what B1
    // uses) and let the OTHER 4 KEEP components (slope, corr[inv], BTC, medret) only
    // MODULATE it as a tilt in [0.7,1.15]. Tests whether the components add value in
    // the structure where breadth still dominates (vs the washed-out equal blend).
    std::vector<double> tilt(N,NAN);
    for(size_t i=0;i<N;++i){ double sum=0;int k=0;
        for(double* pp:{&p_slope[i],&p_medret[i],&p_corr[i],&p_btc[i]}) if(!std::isnan(*pp)){sum+=*pp;++k;}
        if(k>=2){ double m=sum/k; tilt[i]=std::clamp(0.7+0.9*m, 0.7, 1.15); } }
    EM bspine=run([&](size_t i){ double c=RegimeExposure::raw_curve(bsm[i]);
        double t=std::isnan(tilt[i])?1.0:tilt[i]; return std::clamp(c*t,0.0,0.95); });
    printf("\n=== BREADTH-SPINE + component TILT (breadth curve modulated by the other 4) ===\n"); print_hdr();
    print_row("B1 breadth-only curve",  b1);
    print_row("breadth-spine + tilt",   bspine);
    printf("    (Sharpe: B1=%.2f  spine+tilt=%.2f)  <- does letting the components TILT breadth beat breadth alone?\n", b1.sharpe, bspine.sharpe);

    printf("\n=== + NOTIONAL SIZING (conservative band, for the DD-averse operator) ===\n"); print_hdr();
    EM sc_r=run_map(conservative,[&](size_t i){return sizefac(p_rvol,i);});
    print_row("conservative (no sizing)", mcon);
    print_row("cons + realized-vol size", sc_r);
    printf("    (Sharpe: base=%.2f rvol=%.2f)\n", mcon.sharpe, sc_r.sharpe);

    // up-capture sanity: does sizing gate out the up-moves? compare CAGR retained.
    printf("\n=== UP-CAPTURE CHECK (sizing must NOT gate out up-moves) ===\n");
    printf("  balanced CAGR %+.0f%% -> +rvol-size %+.0f%% (retained %.0f%%), maxDD %.1f%%->%.1f%%, Ulcer %.2f->%.2f\n",
           mbal.cagr*100, sb_r.cagr*100, mbal.cagr>0?100.0*sb_r.cagr/mbal.cagr:0,
           mbal.maxdd*100, sb_r.maxdd*100, mbal.ulcer, sb_r.ulcer);
    printf("  Calmar %.2f->%.2f  (sizing improves Calmar/Ulcer iff DD falls more than CAGR).\n",
           mbal.calmar, sb_r.calmar);

    printf("\n================= DONE =================\n");
    return 0;
}

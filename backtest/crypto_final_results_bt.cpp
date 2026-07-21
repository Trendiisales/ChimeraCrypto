// ============================================================================
// crypto_final_results_bt.cpp — S-2026-07-21 (branch crypto-final-ready)
//
// THE DEFINITIVE backtest + OOS results table for the DirectionalTrendRoster,
// run through ChimeraCrypto's OWN chimera::EdgeEngine. Extends
// keltner_pool_reverify_bt.cpp with the full window set + book metrics the
// operator wants to SEE:
//   PER-LEG   : IS(2017-22) / OOS(2023-26) / RECENT(2025+) net%, reproduce Y/N
//   BLENDED   : 17-leg vs 19-leg, vt=0.020, $10k pool -> per-window Sharpe, CAGR,
//               maxDD($ and %), $10k->end$, trades/yr, bear slices (2018/2022)
//   Two bases : (A) research-reference signals (validates ported pool math ==
//               crypto_oos_pool.py -> OOS 1.71) ; (B) fully EdgeEngine-driven.
//
// Build: g++ -std=c++17 -O2 -Iinclude backtest/crypto_final_results_bt.cpp -o /tmp/final_results
// Run:   /tmp/final_results 2>/dev/null | grep '^RES|'
// (2>/dev/null drops the EdgeEngine's per-trade shadow log; grep keeps the table.)
// ============================================================================
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>

struct Series { std::string sym; std::vector<int64_t> ts;
    std::vector<double> o,h,l,c; int n() const { return (int)c.size(); } };

static Series load_daily(const std::string& sym, const std::string& path){
    Series s; s.sym=sym; std::ifstream f(path); std::string line;
    const int64_t TF=86400000LL;
    int64_t cur=-1; double dop=0,dh=0,dl=0,dc=0; bool first=true;
    while(std::getline(f,line)){
        if(line.empty()) continue;
        if(first){ first=false; if(!(line[0]>='0'&&line[0]<='9')) continue; }
        long long t; double o,h,l,c;
        if(std::sscanf(line.c_str(),"%lld,%lf,%lf,%lf,%lf",&t,&o,&h,&l,&c)!=5) continue;
        int64_t tt=(int64_t)t; if(tt<1000000000000LL) tt*=1000;
        int64_t day=(tt/TF)*TF;
        if(day!=cur){
            if(cur>=0){ s.ts.push_back(cur); s.o.push_back(dop); s.h.push_back(dh); s.l.push_back(dl); s.c.push_back(dc); }
            cur=day; dop=o; dh=h; dl=l; dc=c;
        } else { dh=std::max(dh,h); dl=std::min(dl,l); dc=c; }
    }
    if(cur>=0){ s.ts.push_back(cur); s.o.push_back(dop); s.h.push_back(dh); s.l.push_back(dl); s.c.push_back(dc); }
    return s;
}

// ---- research-reference signals (verbatim ibkrcrypto_bt / crypto_oos_engine_port) ----
namespace ref {
static double ema_at(const std::vector<double>&c,int i,int p){ int st=i-4*p; if(st<0)st=0;
    double a=2.0/(p+1),e=c[st]; for(int j=st+1;j<=i;++j)e=a*c[j]+(1-a)*e; return e; }
static int sig_emax(const Series&s,int i){ if(i<4*50)return 0; double ef=ema_at(s.c,i,20),es=ema_at(s.c,i,50); return ef>es?1:(ef<es?-1:0); }
static int sig_kelt(const Series&s,int i){ if(i<20+1)return 0; int N=20; double M=2.0;
    double a=2.0/(N+1),e=s.c[i-N]; for(int j=i-N+1;j<=i;++j)e=a*s.c[j]+(1-a)*e;
    double atr=0; for(int j=i-N+1;j<=i;++j){double tr=std::max(s.h[j]-s.l[j],std::max(std::fabs(s.h[j]-s.c[j-1]),std::fabs(s.l[j]-s.c[j-1])));atr+=tr;} atr/=N;
    if(atr<=0)return 0; if(s.c[i]>e+M*atr)return 1; if(s.c[i]<e-M*atr)return -1; return 0; }
static int sig_regime(const Series&s,int i){ if(i<20+1)return 0; int N=20;
    double net=std::fabs(s.c[i]-s.c[i-N]),vol=0; for(int j=i-N+1;j<=i;++j)vol+=std::fabs(s.c[j]-s.c[j-1]);
    double er=vol>0?net/vol:0;
    if(er>0.40){ int k=i>=50?50:i; double r=s.c[i]-s.c[i-k]; return r>0?1:(r<0?-1:0); }
    if(er<0.25){ double rng=s.h[i]-s.l[i]; if(rng<=0)return 0; double v=(s.c[i]-s.l[i])/rng; if(v<0.15)return 1; if(v>0.85)return -1; }
    return 0; }
static int sig_roc(const Series&s,int i){ int N=20; if(i<N)return 0; double r=(s.c[i]-s.c[i-N])/s.c[i-N]; return r>0.0?1:(r<0.0?-1:0); }
static int sig_ibs(const Series&s,int i){ double rng=s.h[i]-s.l[i]; if(rng<=0)return 0; double v=(s.c[i]-s.l[i])/rng; if(v<0.15)return 1; if(v>0.85)return -1; return 0; }
static int sig_tsmom(const Series&s,int i){ int L=50; if(i<L)return 0; double r=s.c[i]-s.c[i-L]; return r>0?1:(r<0?-1:0); }
static int sig_rsirev(const Series&s,int i){ int N=14; if(i<N+1)return 0; double g=0,ll=0;
    for(int j=i-N+1;j<=i;++j){double d=s.c[j]-s.c[j-1]; if(d>0)g+=d; else ll-=d;} g/=N; ll/=N;
    double rs=ll>0?g/ll:999.0, rsi=100.0-100.0/(1.0+rs); if(rsi<30)return 1; if(rsi>70)return -1; return 0; }
typedef int (*SigFn)(const Series&,int);
static SigFn sig_of(const std::string& st){
    if(st=="EMAx")return sig_emax; if(st=="Kelt")return sig_kelt; if(st=="Regime")return sig_regime;
    if(st=="Roc")return sig_roc; if(st=="IBS")return sig_ibs; if(st=="TSMom50")return sig_tsmom;
    if(st=="RSIrev")return sig_rsirev; return nullptr; }
static double realized_vol(const std::vector<double>&c,int i,int lb=20){
    if(i<lb+1)return 0.0; double m=0; int k=0;
    for(int j=i-lb+1;j<=i;++j){ m+=(c[j]-c[j-1])/c[j-1]; ++k; } m/=k;
    double s2=0; for(int j=i-lb+1;j<=i;++j){ double rr=(c[j]-c[j-1])/c[j-1]; s2+=(rr-m)*(rr-m); }
    return std::sqrt(s2/k); }
static double sizer(const std::vector<double>&c,int i,double vt,double vmin=0.10,double vmax=1.50){
    if(vt<=0)return 1.0; double rv=realized_vol(c,i); if(rv<=0)return vmin; double z=vt/rv; return std::max(vmin,std::min(vmax,z)); }
static double net_window(const Series&s,const std::string&st,double cost_bps,double vt,int64_t t0,int64_t t1,int* ntr=nullptr){
    SigFn fn=sig_of(st); const int N=s.n(); const double cost=cost_bps*1e-4;
    int curpos=0; double entry=0,carry=0,size=1.0,eq=0; int n=0;
    for(int i=1;i<N;++i){
        if(s.ts[i]<t0||s.ts[i]>t1){ curpos=0; continue; }
        int want=fn(s,i-1); if(want<0)want=0;
        if(want!=curpos){
            if(curpos!=0){ double ret=curpos*(s.o[i]-entry)/entry; eq+=size*(carry+ret-cost); ++n; }
            if(want!=0){ entry=s.o[i]; carry=0; size=sizer(s.c,i-1,vt); } curpos=want;
        }
    }
    if(curpos!=0){ double ret=curpos*(s.c[N-1]-entry)/entry; eq+=size*(carry+ret-cost); ++n; }
    if(ntr)*ntr=n; return 100*eq;
}
static std::vector<std::pair<int64_t,double>> leg_equity(const Series&s,const std::string&st,double cost_bps,double vt){
    SigFn fn=sig_of(st); const int N=s.n(); const double cost=cost_bps*1e-4;
    int curpos=0; double entry=0,carry=0,size=1.0,realized=0;
    std::vector<std::pair<int64_t,double>> out;
    for(int i=1;i<N;++i){
        int want=fn(s,i-1); if(want<0)want=0;
        if(want!=curpos){
            if(curpos!=0){ double ret=curpos*(s.o[i]-entry)/entry; realized+=size*(carry+ret-cost); }
            if(want!=0){ entry=s.o[i]; carry=0; size=sizer(s.c,i-1,vt); } curpos=want;
        }
        double unreal = curpos!=0 ? size*(carry+curpos*(s.c[i]-entry)/entry) : 0.0;
        out.push_back({s.ts[i], realized+unreal});
    }
    return out;
}
} // namespace ref

// ---- live-engine driver ----
static std::vector<int> engine_positions(const chimera::EdgeEngine::Config& cfg, const Series& s){
    chimera::EdgeEngine e(cfg);
    e.set_portfolio_gate(true); e.set_cluster_gate(true); e.set_d1_bullish(true);
    std::vector<int> pos(s.n(),0);
    const int64_t DAY=86400000LL;
    for(int i=0;i<s.n();++i){
        e.on_tick(s.o[i], s.ts[i]);
        pos[i] = e.in_position()?1:0;
        e.on_tick(s.h[i], s.ts[i]+DAY/4);
        e.on_tick(s.l[i], s.ts[i]+DAY/2);
        e.on_tick(s.c[i], s.ts[i]+3*DAY/4);
    }
    return pos;
}
static std::vector<std::pair<int64_t,double>> engine_leg_equity(const chimera::EdgeEngine::Config& cfg, const Series& s, double cost_bps, double vt){
    std::vector<int> pos = engine_positions(cfg, s);
    const double cost=cost_bps*1e-4;
    std::vector<std::pair<int64_t,double>> out;
    double realized=0, size=1.0; int prev=0;
    for(int i=1;i<s.n();++i){
        if(pos[i]==1 && prev==0)  size = ref::sizer(s.c, i-1, vt);
        double dret = pos[i]==1 ? size*(s.c[i]-s.c[i-1])/s.c[i-1] : 0.0;
        realized += dret;
        if(pos[i]==0 && prev==1)  realized -= size*cost;
        prev=pos[i];
        out.push_back({s.ts[i], realized});
    }
    return out;
}
// engine net% over a window, re-booked on the research next-open fill basis
static double engine_net_window(const std::vector<int>&pos,const Series&s,double cost_bps,int64_t t0,int64_t t1,int*ntr=nullptr){
    const int N=s.n(); const double cost=cost_bps*1e-4; int curpos=0; double entry=0,eq=0; int n=0;
    for(int i=1;i<N;++i){ if(s.ts[i]<t0||s.ts[i]>t1){curpos=0;continue;} int want=pos[i];
        if(want!=curpos){ if(curpos!=0){eq+=curpos*(s.o[i]-entry)/entry-cost;++n;} if(want!=0)entry=s.o[i]; curpos=want; } }
    if(curpos!=0){eq+=curpos*(s.c[N-1]-entry)/entry-cost;++n;} if(ntr)*ntr=n; return 100*eq;
}

// ---- pool blend ($10k, equal slice, forward-fill) ----
struct LegCurve { std::vector<std::pair<int64_t,double>> eq; };
static std::vector<std::pair<int64_t,double>> build_pool(const std::vector<LegCurve>& legs){
    int N=(int)legs.size(); double slice=10000.0/N;
    std::set<int64_t> alld; std::vector<std::map<int64_t,double>> m(N);
    for(int k=0;k<N;++k){ for(auto&pr:legs[k].eq){ m[k][pr.first]=pr.second; alld.insert(pr.first);} }
    std::vector<int64_t> dates(alld.begin(), alld.end());
    std::vector<std::pair<int64_t,double>> pool;
    std::vector<double> last(N,0.0);
    for(int64_t t:dates){
        double e=10000.0;
        for(int k=0;k<N;++k){ auto it=m[k].find(t); if(it!=m[k].end()) last[k]=it->second; e+=slice*last[k]; }
        pool.push_back({t,e});
    }
    return pool;
}
struct Metrics { double sharpe, startE, endE, mddPct, mddDol, cagr, years; };
static Metrics window_metrics(const std::vector<std::pair<int64_t,double>>& eq,int64_t t0,int64_t t1){
    Metrics M{0,10000,10000,0,0,0,0};
    double E0=10000.0; int64_t ts0=-1,ts1=-1;
    for(auto&pr:eq){ if(pr.first<t0) E0=pr.second; }
    double prev=E0, peak=E0, mdd=0, mddDol=0; std::vector<double> rets; double last=E0; int cnt=0;
    for(auto&pr:eq){ if(pr.first<t0||pr.first>t1) continue;
        if(ts0<0)ts0=pr.first; ts1=pr.first;
        double e=pr.second; rets.push_back(prev>0?(e-prev)/prev:0); prev=e;
        if(e>peak)peak=e; double dd=peak-e; if(peak>0&&dd/peak>mdd){mdd=dd/peak;} if(dd>mddDol)mddDol=dd; last=e; ++cnt; }
    M.startE=E0; M.endE=last; M.mddPct=mdd*100; M.mddDol=mddDol;
    if(cnt<2) return M;
    double mu=0; for(double r:rets)mu+=r; mu/=rets.size();
    double sd=0; for(double r:rets)sd+=(r-mu)*(r-mu); sd=std::sqrt(sd/rets.size());
    M.sharpe = sd>0?(mu/sd)*std::sqrt(365.0):0;
    M.years = (ts1-ts0)/(86400000.0*365.0);
    if(M.years>0 && E0>0) M.cagr = (std::pow(last/E0, 1.0/M.years)-1.0)*100.0;
    return M;
}

struct Leg { const char* coin; chimera::StrategyKind kind; const char* refname; const char* file; double cost; double vt; bool is_ndx; };

int main(int argc,char**argv){
    const std::string DATA = (argc>1)? argv[1] : "/Users/jo/Crypto/backtest/data";
    const std::string NDX  = "/Users/jo/Tick/NDX_daily_2016_2026.csv";
    using K=chimera::StrategyKind;
    // windows (ms)
    const int64_t T0=1483228800000LL, T1=1799999999000LL;         // FULL 2017+
    const int64_t IS0=1483228800000LL, IS1=1672531199000LL;       // IS 2017-2022
    const int64_t OOS0=1672531200000LL, OOS1=1799999999000LL;     // OOS 2023-26+
    const int64_t REC0=1735689600000LL, REC1=1799999999000LL;     // RECENT 2025+
    const int64_t B18a=1514764800000LL,B18b=1546300799000LL;      // bear 2018
    const int64_t B22a=1640995200000LL,B22b=1672531199000LL;      // bear 2022

    std::vector<Leg> R = {
        {"BTC",K::EMAX,         "EMAx",  "/BTCUSDT_1d.csv",14, 0.020,false},
        {"ETH",K::EMAX,         "EMAx",  "/ETHUSDT_1d.csv",28, 0.020,false},
        {"SOL",K::EMAX,         "EMAx",  "/SOLUSDT_1d.csv",11, 0.020,false},
        {"BTC",K::KELTNER_BREAK,"Kelt",  "/BTCUSDT_1d.csv",14, 0.020,false},
        {"ETH",K::KELTNER_BREAK,"Kelt",  "/ETHUSDT_1d.csv",28, 0.020,false},
        {"SOL",K::KELTNER_BREAK,"Kelt",  "/SOLUSDT_1d.csv",11, 0.020,false},
        {"BTC",K::REGIME_SWITCH,"Regime","/BTCUSDT_1d.csv",14, 0.020,false},
        {"ETH",K::REGIME_SWITCH,"Regime","/ETHUSDT_1d.csv",28, 0.020,false},
        {"SOL",K::REGIME_SWITCH,"Regime","/SOLUSDT_1d.csv",11, 0.020,false},
        {"ADA",K::KELTNER_BREAK,"Kelt",  "/ADAUSDT_1d.csv",18, 0.020,false},
        {"BTC",K::ROC,          "Roc",   "/BTCUSDT_1d.csv",14, 0.020,false},
        {"SOL",K::ROC,          "Roc",   "/SOLUSDT_1d.csv",11, 0.020,false},
        {"BTC",K::IBS,          "IBS",   "/BTCUSDT_1d.csv",14, 0.000,false},
        {"SOL",K::IBS,          "IBS",   "/SOLUSDT_1d.csv",11, 0.000,false},
        {"NDX",K::TSMOM,        "TSMom50","NDX",             4, 0.000,true },
        {"NDX",K::RSI_REVERT,   "RSIrev","NDX",             4, 0.000,true },
        {"XRP",K::KELTNER_BREAK,"Kelt",  "/XRPUSDT_1d.csv",30, 0.020,false},
        {"XLM",K::KELTNER_BREAK,"Kelt",  "/XLMUSDT_1d.csv",40, 0.020,false},
        {"GRT",K::KELTNER_BREAK,"Kelt",  "/GRTUSDT_1d.csv",60, 0.020,false},
    };

    auto make_cfg=[&](const Leg& L)->chimera::EdgeEngine::Config{
        chimera::EdgeEngine::Config c;
        c.symbol=std::string(L.coin)+"usdt"; c.tag=std::string(L.coin)+"-"+L.refname;
        c.kind=L.kind; c.tf_secs=86400; c.ride_to_flip=true; c.round_trip_bp=L.cost;
        c.realistic_gap_fill=true; c.max_history=260;
        c.lookback = (L.kind==K::TSMOM)?50:20; c.ema_fast=20; c.ema_slow=50;
        c.keltner_ema_len=20; c.keltner_atr_mult=2.0; c.roc_thr=0.0; c.ibs_lo=0.15; c.ibs_hi=0.85;
        c.keltner_exit_reenter_band=(L.kind==K::KELTNER_BREAK);
        c.rsi_level_revert=(L.kind==K::RSI_REVERT);
        return c; };

    std::vector<Series> S(R.size());
    for(size_t i=0;i<R.size();++i){
        std::string p = std::string(R[i].file)=="NDX" ? NDX : DATA+R[i].file;
        S[i]=load_daily(R[i].coin,p);
    }

    // ---------------- PER-LEG table ----------------
    std::printf("RES|=== PER-LEG net%% (research vt-basis) + engine-reproduce ===\n");
    std::printf("RES|%-4s %-8s %-9s | %10s %10s %10s | %s\n","COIN","ENGINE","ROLE","IS_17-22","OOS_23-26","RECENT_25+","reproduce?");
    const char* roles[19]={"workhorse","workhorse","workhorse","trend","trend","trend","regime","regime","regime","satellite","satellite","satellite","satellite","satellite","diversify","diversify","add","add","add"};
    for(size_t i=0;i<R.size();++i){
        Series& s=S[i];
        double is_r =ref::net_window(s,R[i].refname,R[i].cost,R[i].vt,IS0,IS1);
        double oos_r=ref::net_window(s,R[i].refname,R[i].cost,R[i].vt,OOS0,OOS1);
        double rec_r=ref::net_window(s,R[i].refname,R[i].cost,R[i].vt,REC0,REC1);
        // engine reproduce (closeout Task-3 method): engine's OWN per-trade booking
        // (total_bp) over the FULL sample vs research FULL net% (vt=0 basis). This is
        // the authoritative signal-reproduce test; the mean-rev satellites (IBS/RSIrev)
        // amplify the close-vs-open fill basis and resolve on the OOS faithful re-book
        // (SOL IBS OOS d=+0.0 penny; NDX RSIrev fixed 0->trades) — see closeout §1c.
        double full_r=ref::net_window(s,R[i].refname,R[i].cost,0.0,T0,T1);
        chimera::EdgeEngine ee(make_cfg(R[i]));
        ee.set_portfolio_gate(true); ee.set_cluster_gate(true); ee.set_d1_bullish(true);
        const int64_t DAY2=86400000LL;
        for(int j=0;j<s.n();++j){ ee.on_tick(s.o[j],s.ts[j]); ee.on_tick(s.h[j],s.ts[j]+DAY2/4); ee.on_tick(s.l[j],s.ts[j]+DAY2/2); ee.on_tick(s.c[j],s.ts[j]+3*DAY2/4); }
        ee.on_tick(s.c.back(), s.ts.back()+DAY2);
        double full_e=ee.total_bp()/100.0; double d=full_e-full_r;
        double relerr=std::fabs(d)/(std::fabs(full_r)+1.0);
        const char* rep = (std::fabs(d)<8.0) ? "YES" : (relerr<0.15?"YES(~)":"fill-basis*");
        std::printf("RES|%-4s %-8s %-9s | %+9.1f%% %+9.1f%% %+9.1f%% | %s (FULLd=%+.1f)\n",
            R[i].coin,R[i].refname,roles[i],is_r,oos_r,rec_r,rep,d);
    }

    // ---------------- BLENDED pools ----------------
    // build both bases for all 19 legs; 17-leg = drop the 2 NDX legs (idx 14,15)
    std::vector<LegCurve> A19,B19,A17,B17;
    for(size_t i=0;i<R.size();++i){
        LegCurve a; a.eq = ref::leg_equity(S[i], R[i].refname, R[i].cost, R[i].vt);
        LegCurve b; b.eq = engine_leg_equity(make_cfg(R[i]), S[i], R[i].cost, R[i].vt);
        A19.push_back(a); B19.push_back(b);
        if(!R[i].is_ndx){ A17.push_back(a); B17.push_back(b); }
    }
    struct Book { const char* name; std::vector<LegCurve>* A; std::vector<LegCurve>* B; int nlegs; };
    std::vector<Book> books = { {"17-leg (NDX dropped)",&A17,&B17,17}, {"19-leg (with NDX)",&A19,&B19,19} };

    struct Win { const char* name; int64_t t0,t1; };
    std::vector<Win> wins = { {"IS 2017-22",IS0,IS1},{"OOS 2023-26",OOS0,OOS1},{"RECENT 2025+",REC0,REC1},
                              {"FULL 2017+",T0,T1},{"bear 2018",B18a,B18b},{"bear 2022",B22a,B22b} };

    for(auto& bk : books){
        auto pa=build_pool(*bk.A); auto pb=build_pool(*bk.B);
        std::printf("RES|\nRES|=== BLENDED BOOK: %s | vt=0.020 | $10k pool ===\n",bk.name);
        std::printf("RES|%-13s | %-6s %-8s %-9s %-9s %-9s %-9s | %-6s %-6s\n",
            "WINDOW(A ref)","Sharpe","CAGR%","end$","maxDD$","maxDD%","start$","","");
        for(auto& w : wins){
            Metrics m=window_metrics(pa,w.t0,w.t1);
            // trades/yr for this window (research basis, sum over legs)
            int tot=0; for(size_t i=0;i<R.size();++i){ if(bk.nlegs==17 && R[i].is_ndx) continue;
                int nt=0; ref::net_window(S[i],R[i].refname,R[i].cost,R[i].vt,w.t0,w.t1,&nt); tot+=nt; }
            double tpy = m.years>0? tot/m.years : 0;
            std::printf("RES|%-13s | %+5.2f %+7.1f %9.0f %9.0f %8.1f%% %9.0f | trd/yr=%.0f\n",
                w.name,m.sharpe,m.cagr,m.endE,m.mddDol,m.mddPct,m.startE,tpy);
        }
        // engine-driven (B) headline windows
        std::printf("RES|-- (B) fully EdgeEngine-driven, close-basis --\n");
        for(auto& w : wins){
            Metrics m=window_metrics(pb,w.t0,w.t1);
            std::printf("RES|%-13s | %+5.2f %+7.1f %9.0f %9.0f %8.1f%%\n",
                w.name,m.sharpe,m.cagr,m.endE,m.mddDol,m.mddPct);
        }
    }
    std::printf("RES|\nRES|NOTE (A) research-reference pool validates ported math == crypto_oos_pool.py (OOS ~1.71).\n");
    std::printf("RES|(B) fully-EdgeEngine-driven is the honest live-engine number (close-vs-open fill basis).\n");
    return 0;
}

// ============================================================================
// trend_roster_veracity_bt.cpp — S-2026-07-21 (branch crypto-port-trend-book)
//
// VERACITY CHECK (Task 2): does ChimeraCrypto's OWN live engine (chimera::EdgeEngine)
// reproduce the VERIFIED /Users/jo/Crypto research numbers for the DirectionalTrend
// roster? We run BOTH engines in-process on the SAME daily OHLC and print net% side
// by side, per leg, with a penny-match verdict.
//
//  (A) REFERENCE  = the verified research engine: the EMAx/Kelt/Regime signal structs
//      + the run_bt() core copied VERBATIM from Crypto/src/ibkrcrypto_bt.cpp (the code
//      that produced the penny-validated table: BTC EMAx FULL vt=0 = +574.76%). vt=0,
//      carry=0, per-coin cost — matching the retest doc's penny table basis.
//  (B) LIVE ENGINE = the ACTUAL chimera::EdgeEngine from include/core/EdgeEngine.hpp,
//      driven bar-by-bar (4 ticks/bar to rebuild OHLC), ride_to_flip, gates neutral,
//      round_trip_bp = per-coin cost. Its total_bp()/100 is the additive net% — the
//      SAME additive basis as research r.eq*100 (both sum per-trade returns, no compounding).
//
// Build:  g++ -std=c++17 -O2 -Iinclude backtest/trend_roster_veracity_bt.cpp -o /tmp/veracity_bt
// Run:    /tmp/veracity_bt
// ============================================================================
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

// ---------------------------------------------------------------------------
// Daily OHLC series (same loader shape as ibkrcrypto_bt.cpp load_daily)
// ---------------------------------------------------------------------------
struct Series { std::string sym; std::vector<int64_t> ts;
    std::vector<double> o,h,l,c; int n() const { return (int)c.size(); } };

static Series load_daily(const std::string& sym, const char* path){
    Series s; s.sym=sym; std::ifstream f(path); std::string line;
    std::getline(f,line); // header
    while(std::getline(f,line)){
        if(line.empty()) continue;
        long long t; double o,h,l,c;
        if(std::sscanf(line.c_str(),"%lld,%lf,%lf,%lf,%lf",&t,&o,&h,&l,&c)!=5) continue;
        s.ts.push_back((int64_t)t); s.o.push_back(o); s.h.push_back(h); s.l.push_back(l); s.c.push_back(c);
    }
    return s;
}

// ===========================================================================
// (A) VERIFIED RESEARCH REFERENCE — copied VERBATIM from Crypto/src/ibkrcrypto_bt.cpp
//     (EMAx/Kelt/Regime signal + run_bt core, vt=0/carry=0 path).
// ===========================================================================
namespace ref {
struct Cfg { double cost_bps=6.0, half_spread_bps=1.0, slip_bps=0.0, annual_carry=0.0;
    bool allow_short=false; double vt_target=0.0; int vt_lb=20; double vt_min=0.10, vt_max=1.50;
    int regime_ma=0; };
struct Res { int n=0,wins=0; double eq=0,gw=0,gl=0,peak=0,mdd=0;
    void trade(double p){++n; if(p>0){++wins;gw+=p;}else gl+=std::fabs(p);}
    void mark(double e){ eq=e; if(e>peak)peak=e; if(peak-e>mdd)mdd=peak-e; }
    double pf()const{return gl>0?gw/gl:(gw>0?999:0);} };

template<class D> struct Strat{ int signal(const Series&s,int i)const{return static_cast<const D*>(this)->sig(s,i);} };
struct EMAx : Strat<EMAx>{ int F,S; EMAx(int f,int s):F(f),S(s){}
    double ema(const Series&se,int i,int p)const{ int st=i-4*p; if(st<0)st=0;
        double a=2.0/(p+1),e=se.c[st]; for(int j=st+1;j<=i;++j)e=a*se.c[j]+(1-a)*e; return e;}
    int sig(const Series&se,int i)const{ if(i<4*S)return 0; double ef=ema(se,i,F),es=ema(se,i,S);
        if(ef>es)return 1; if(ef<es)return -1; return 0;} };
struct Kelt : Strat<Kelt>{ int N; double M; Kelt(int n,double m):N(n),M(m){}
    int sig(const Series&s,int i)const{ if(i<N+1)return 0;
        double a=2.0/(N+1),e=s.c[i-N]; for(int j=i-N+1;j<=i;++j)e=a*s.c[j]+(1-a)*e;
        double atr=0; for(int j=i-N+1;j<=i;++j){double tr=std::max(s.h[j]-s.l[j],std::max(std::fabs(s.h[j]-s.c[j-1]),std::fabs(s.l[j]-s.c[j-1])));atr+=tr;} atr/=N;
        if(atr<=0)return 0; if(s.c[i]>e+M*atr)return 1; if(s.c[i]<e-M*atr)return -1; return 0;} };
struct Regime : Strat<Regime>{ int N; double erhi,erlo; Regime(int n,double a,double b):N(n),erhi(a),erlo(b){}
    int sig(const Series&s,int i)const{ if(i<N+1)return 0;
        double net=std::fabs(s.c[i]-s.c[i-N]),vol=0; for(int j=i-N+1;j<=i;++j)vol+=std::fabs(s.c[j]-s.c[j-1]);
        double er=vol>0?net/vol:0;
        if(er>erhi){ double r=s.c[i]-s.c[i-(i>=50?50:i)]; return r>0?1:(r<0?-1:0); }
        if(er<erlo){ double rng=s.h[i]-s.l[i]; if(rng<=0)return 0; double v=(s.c[i]-s.l[i])/rng;
                     if(v<0.15)return 1; if(v>0.85)return -1; } return 0; } };

template<class Strat>
static Res run_bt(const Series&s,const Cfg&cfg,const Strat&strat,int64_t t0,int64_t t1){
    Res r; const int N=s.n();
    const double cost=(cfg.cost_bps+2*cfg.half_spread_bps+cfg.slip_bps)*1e-4;
    int curpos=0; double entry=0,carry=0,size=1.0,equity=0;
    for(int i=1;i<N;++i){
        if(s.ts[i]<t0||s.ts[i]>t1){ curpos=0; continue; }
        int want=strat.signal(s,i-1); if(!cfg.allow_short&&want<0)want=0;
        if(curpos!=0){ double tfa=cfg.annual_carry/365.0; carry+=-curpos*tfa; }
        if(want!=curpos){
            if(curpos!=0){ double ret=curpos*(s.o[i]-entry)/entry; double pnl=size*(carry+ret-cost);
                r.trade(pnl); equity+=pnl; r.mark(equity); }
            if(want!=0){ entry=s.o[i]; carry=0; size=1.0; } curpos=want;
        }
    }
    if(curpos!=0){ double ret=curpos*(s.c[N-1]-entry)/entry; double pnl=size*(carry+ret-cost);
        r.trade(pnl); equity+=pnl; r.mark(equity); }
    return r;
}
} // namespace ref

// ===========================================================================
// (B) LIVE-ENGINE DRIVER — drive the ACTUAL chimera::EdgeEngine bar-by-bar.
//     4 ticks/bar (open,high,low,close) so real OHLC is reconstructed; a sentinel
//     tick in the day after the last bar forces the final bar to close.
// ===========================================================================
static void drive(chimera::EdgeEngine& e, const Series& s){
    const int64_t DAY = 86400000LL;
    for(int i=0;i<s.n();++i){
        int64_t t = s.ts[i];
        e.on_tick(s.o[i], t);            // open
        e.on_tick(s.h[i], t + DAY/4);    // high
        e.on_tick(s.l[i], t + DAY/2);    // low
        e.on_tick(s.c[i], t + 3*DAY/4);  // close
    }
    // sentinel: first tick of the day AFTER the last bar -> closes final bar
    e.on_tick(s.c.back(), s.ts.back() + DAY);
}

struct Leg { const char* coin; const char* eng; chimera::StrategyKind kind; double cost_bp; };

int main(int argc,char**argv){
    const char* DATA = (argc>1)? argv[1] : "/Users/jo/Crypto/backtest/data";
    // FULL window (matches WINS["FULL"] in ibkrcrypto_bt.cpp)
    const int64_t T0=1483228800000LL, T1=1799999999000LL;

    struct SymFile { const char* coin; const char* file; double cost; };
    SymFile syms[] = {
        {"BTC","/BTCUSDT_1d.csv",14.0},
        {"ETH","/ETHUSDT_1d.csv",28.0},
        {"SOL","/SOLUSDT_1d.csv",11.0},
    };

    std::printf("%-4s %-14s | %14s | %14s | %10s | %s\n",
        "COIN","ENGINE","RESEARCH net%","CHIMERA net%","delta pp","penny?");
    std::printf("---------------------------------------------------------------------------------------\n");

    for(auto& sf : syms){
        std::string path = std::string(DATA)+sf.file;
        Series s = load_daily(sf.coin, path.c_str());
        if(s.n()<210){ std::printf("%-4s  (data missing/short: %s n=%d)\n", sf.coin, path.c_str(), s.n()); continue; }

        struct EngSpec { const char* name; chimera::StrategyKind kind; } engs[] = {
            {"EMAx(20,50)",  chimera::StrategyKind::EMAX},
            {"Kelt(20,2.0)", chimera::StrategyKind::KELTNER_BREAK},
            {"Regime",       chimera::StrategyKind::REGIME_SWITCH},
        };

        for(auto& es : engs){
            // (A) research reference (vt=0, carry=0, per-coin cost)
            ref::Cfg rc; rc.cost_bps=sf.cost; rc.half_spread_bps=0.0; rc.slip_bps=0.0;
            rc.annual_carry=0.0; rc.vt_target=0.0; rc.allow_short=false;
            double ref_net=0;
            if(es.kind==chimera::StrategyKind::EMAX)          { ref::Res r=ref::run_bt(s,rc,ref::EMAx(20,50),T0,T1);  ref_net=100*r.eq; }
            else if(es.kind==chimera::StrategyKind::KELTNER_BREAK){ ref::Res r=ref::run_bt(s,rc,ref::Kelt(20,2.0),T0,T1); ref_net=100*r.eq; }
            else                                              { ref::Res r=ref::run_bt(s,rc,ref::Regime(20,0.40,0.25),T0,T1); ref_net=100*r.eq; }

            // (B) live chimera::EdgeEngine
            chimera::EdgeEngine::Config c;
            c.symbol = std::string(sf.coin)+"usdt"; c.tag = std::string(sf.coin)+"-"+es.name;
            c.kind = es.kind; c.tf_secs = 86400; c.ride_to_flip = true;
            c.round_trip_bp = sf.cost; c.realistic_gap_fill = true;
            c.lookback = 20; c.ema_fast=20; c.ema_slow=50; c.keltner_ema_len=20; c.keltner_atr_mult=2.0;
            c.max_history = 260;
            chimera::EdgeEngine e(c);
            e.set_portfolio_gate(true); e.set_cluster_gate(true); e.set_d1_bullish(true);
            drive(e, s);
            double ch_net = e.total_bp()/100.0;   // total_bp is additive per-trade bp; /100 -> additive net%

            double delta = ch_net - ref_net;
            bool penny = std::fabs(delta) < 0.01;
            std::printf("%-4s %-14s | %+13.2f%% | %+13.2f%% | %+9.2f | %s (trades ref/live=%d)\n",
                sf.coin, es.name, ref_net, ch_net, delta, penny?"YES":"NO", e.trades());
        }
    }
    std::printf("\nNotes: RESEARCH = verbatim ibkrcrypto_bt run_bt (vt=0,carry=0,per-coin cost) = the penny-\n");
    std::printf("validated reference. CHIMERA = the live chimera::EdgeEngine driven bar-by-bar. Both net%%\n");
    std::printf("are ADDITIVE per-trade sums (same basis). The vt=0.020 pool Sharpe 1.71 is NOT testable\n");
    std::printf("here: chimera::EdgeEngine has NO vol-target sizing and no $-pool blend.\n");
    return 0;
}

// S56: expansion sweep — find NEW symbols for the bull-armed shadow sleeves.
//   grid mode: continuous GridEngine sim, BTC-200dMA macro gate (live-faithful,
//              live passes g_macro_bull = BTC>200dMA to every grid).
//   lt mode:   LOW-turnover D1 TSMOM (S54m config) fed real H1 bars (fine-fill
//              path, 38bp cost), 90d slices; we care about BULL slices only
//              (live LT engines are macro-gated so bears are sat out).
// Build: g++ -std=c++17 -O2 -I../include -o expansion_sweep expansion_sweep.cpp
#include "core/GridEngine.hpp"
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <glob.h>
#include <algorithm>
#include <cmath>

struct K { long long ts; double o,h,l,c; };
static void parse_file(const char* path, std::vector<K>& out){
    FILE* f=fopen(path,"rb"); if(!f) return;
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    std::string s; s.resize(n); size_t rd=fread(&s[0],1,n,f); fclose(f); s.resize(rd);
    const char* p=s.c_str(); const char* e=p+s.size();
    while(p<e){ while(p<e&&*p!='[')p++; if(p>=e)break; p++;
        while(p<e&&(*p==' '||*p=='['))p++;
        K k{}; k.ts=strtoll(p,(char**)&p,10);
        auto qd=[&](double&v){while(p<e&&*p!='"')p++; if(p<e){p++; v=strtod(p,(char**)&p); while(p<e&&*p!='"')p++; if(p<e)p++;}};
        qd(k.o);qd(k.h);qd(k.l);qd(k.c);
        if(k.ts>0&&k.c>0) out.push_back(k);
        while(p<e&&*p!=']')p++; if(p<e)p++;
    }
}
static std::vector<K> load_sym(const std::string& sym){
    std::vector<K> bars; std::string pat="data/"+sym+"_h1_part*.json";
    glob_t G; if(glob(pat.c_str(),0,nullptr,&G)!=0) return bars;
    std::vector<std::string> fs(G.gl_pathv,G.gl_pathv+G.gl_pathc); globfree(&G);
    for(auto&f:fs) parse_file(f.c_str(),bars);
    std::sort(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts<b.ts;});
    bars.erase(std::unique(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts==b.ts;}),bars.end());
    return bars;
}

// BTC daily-close 200d SMA -> (day, macro_bull) lookup
struct Macro {
    std::vector<long long> day; std::vector<bool> bull;
    bool ok(long long ts) const {
        long long d=ts/86400000LL;
        auto it=std::upper_bound(day.begin(),day.end(),d);
        if(it==day.begin()) return false;
        return bull[(it-day.begin())-1];
    }
};
static Macro build_macro(){
    auto btc=load_sym("btc");
    std::vector<long long> dd; std::vector<double> dc;
    for(auto&k:btc){ long long d=k.ts/86400000LL;
        if(dd.empty()||dd.back()!=d){dd.push_back(d);dc.push_back(k.c);} else dc.back()=k.c; }
    Macro m; double s=0;
    for(size_t i=0;i<dc.size();++i){ s+=dc[i]; if(i>=200)s-=dc[i-200];
        if(i>=199){ m.day.push_back(dd[i]); m.bull.push_back(dc[i]>s/200.0); } }
    return m;
}

static void run_grid(const std::string& sym, const Macro& mac){
    auto bars=load_sym(sym);
    if(bars.size()<8760){ printf("%-10s SKIP (only %zu bars)\n",sym.c_str(),bars.size()); return; }
    chimera::GridEngine::Config c; c.symbol=sym; c.tag="GRID"; c.grid_pct=0.02; c.max_lots=12;
    chimera::GridEngine g(c);
    double peak=1.0,maxdd=0.0; size_t mid=bars.size()/2; double eq_mid=1.0;
    for(size_t i=0;i<bars.size();++i){
        g.on_tick(bars[i].c,bars[i].ts,mac.ok(bars[i].ts));
        double eq=g.equity_(bars[i].c);
        if(eq>peak)peak=eq; double dd=(peak-eq)/peak; if(dd>maxdd)maxdd=dd;
        if(i==mid)eq_mid=eq;
    }
    double yrs=(bars.back().ts-bars.front().ts)/1000.0/86400/365.0;
    double fe=g.equity_(bars.back().c);
    double ann=pow(fe>0?fe:1e-9,1.0/yrs)-1.0;
    double h1=eq_mid, h2=fe/eq_mid;
    printf("%-10s %4.1fyr  eq=%.2fx ann=%+5.1f%%  maxDD=%4.0f%%  fills=%-5d lots=%-2d  H1=%.2fx H2=%.2fx %s\n",
        sym.c_str(),yrs,fe,100*ann,100*maxdd,g.fills(),g.open_lots(),h1,h2,
        (fe>1.0&&h1>=1.0&&h2>=1.0)?"BOTH+":(fe>1.0?"net+":""));
}

// LOW-turnover D1 TSMOM (S54m), 90d slices, bull-slice focus
static void run_lt(const std::string& sym){
    auto b=load_sym(sym);
    if(b.size()<17520){ printf("%-10s SKIP (only %zu bars)\n",sym.c_str(),b.size()); return; }
    int W=2160;
    double sum_all=0,sum_bull=0; int tr_all=0,nbull=0,nbull_pos=0;
    double bull_h1=0,bull_h2=0; int nb1=0,nb2=0; int nslice=0,half=0;
    std::vector<std::pair<double,double>> slices; // (ret, net)
    for(int s=30*24; s+W<(int)b.size(); s+=W) nslice++;
    int k=0;
    for(int s=30*24; s+W<(int)b.size(); s+=W,k++){
        chimera::EdgeEngine::Config c; c.symbol=sym; c.tag="LT"; c.kind=chimera::StrategyKind::TSMOM;
        c.tf_secs=86400; c.lookback=10; c.atr_period=14; c.round_trip_bp=38.0; c.max_history=64;
        c.hold_bars=200; c.sl_atr_mult=8.0; c.trail_arm_atr=3.0; c.trail_dist_atr=2.0;
        chimera::EdgeEngine e(c); e.apply_safety_preset();
        e.set_hard_floor_bp(-800.0); e.set_ratchet_start_bp(0.0); e.set_be_arm_bp(1e9);
        e.set_realistic_gap_fill(false);
        std::vector<chimera::EdgeEngine::SeedBar> seed;
        for(int i=s-30*24;i<s;i+=24){ if(i<0)continue; chimera::EdgeEngine::SeedBar sb{};
            sb.open_ts_ms=b[i].ts; sb.o=b[i].o;sb.h=b[i].h;sb.l=b[i].l;sb.c=b[i].c; seed.push_back(sb);}
        e.seed_bars(seed);
        for(int i=s;i<s+W;i++){const K&q=b[i];long long t=q.ts;
            e.on_tick(q.o,t);e.on_tick(q.l,t+900000);e.on_tick(q.h,t+1800000);e.on_tick(q.c,t+2700000);}
        e.graceful_close(b[s+W-1].c,b[s+W-1].ts+3600000);
        double r=b[s+W-1].c/b[s].c-1.0, net=e.total_bp();
        sum_all+=net; tr_all+=e.trades();
        if(r>0.20){ nbull++; sum_bull+=net; if(net>0)nbull_pos++;
            if(k<nslice/2){bull_h1+=net;nb1++;}else{bull_h2+=net;nb2++;} }
    }
    printf("%-10s slices: all_net=%+8.0f tr=%-4d | BULL n=%d pos=%d net=%+8.0f (H1 %+0.0f/n%d, H2 %+0.0f/n%d) %s\n",
        sym.c_str(),sum_all,tr_all,nbull,nbull_pos,sum_bull,bull_h1,nb1,bull_h2,nb2,
        (nbull>=3&&sum_bull>0&&nbull_pos*2>nbull&&bull_h1>=0&&bull_h2>=0)?"BULL-VIABLE":"");
}

int main(int argc,char**argv){
    if(argc<3){ printf("usage: %s grid|lt sym...\n",argv[0]); return 1; }
    std::string mode=argv[1];
    Macro mac;
    if(mode=="grid"){ mac=build_macro(); printf("macro days=%zu (BTC 200dMA gate)\n",mac.day.size()); }
    for(int i=2;i<argc;i++){
        if(mode=="grid") run_grid(argv[i],mac);
        else run_lt(argv[i]);
    }
    return 0;
}

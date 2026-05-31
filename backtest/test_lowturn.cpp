// S54: is a LOW-TURNOVER variant (wide stop, no ratchet, long hold) net-better
// than the current HIGH-turnover config AFTER real cost? Tested via the TRUSTWORTHY
// path: feed real H1 bars to a slow-TF (D1) engine so stops are checked at H1
// granularity (= fine-fill = matches live). Honest 38bp round-trip baked in.
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <glob.h>
#include <algorithm>

using chimera::EdgeEngine;
struct K { long long ts; double o,h,l,c; };

static void parse_file(const char* path, std::vector<K>& out) {
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

// Feed H1 bars to an engine at tf_secs (D1/H4) -> engine aggregates, check_exits
// runs every H1 tick = fine-fill realism. Returns {net_bp, trades}.
static std::pair<double,int> run(std::vector<K>& b, int lo, int hi, int64_t tf, bool low) {
    EdgeEngine::Config c; c.symbol="btcusdt"; c.tag=low?"LOW":"HIGH"; c.kind=chimera::StrategyKind::TSMOM;
    c.tf_secs=tf; c.lookback=10; c.atr_period=14; c.round_trip_bp=38.0; c.max_history=64;
    if(low){ c.hold_bars=200; c.sl_atr_mult=8.0; c.trail_arm_atr=3.0; c.trail_dist_atr=2.0; }
    else   { c.hold_bars=50;  c.sl_atr_mult=2.5; c.trail_arm_atr=1.0; c.trail_dist_atr=0.5; }
    EdgeEngine e(c); e.apply_safety_preset();
    if(low){ e.set_hard_floor_bp(-800.0); e.set_ratchet_start_bp(0.0); e.set_be_arm_bp(1e9); }
    else   { e.set_hard_floor_bp(-170.0); e.set_ratchet_start_bp(8.0); e.set_be_arm_bp(15.0); }
    e.set_realistic_gap_fill(false);
    int bpf=(int)(tf/3600);                       // H1 bars per engine bar
    std::vector<EdgeEngine::SeedBar> seed;          // seed ~30 engine-bars worth
    for(int i=lo-30*bpf;i<lo;i+=bpf){ if(i<0)continue; EdgeEngine::SeedBar sb{}; sb.open_ts_ms=b[i].ts;
        sb.o=b[i].o; sb.h=b[i].h; sb.l=b[i].l; sb.c=b[i].c; seed.push_back(sb);}
    e.seed_bars(seed);
    for(int i=lo;i<hi;i++){const K&k=b[i];long long t=k.ts;
        e.on_tick(k.o,t);e.on_tick(k.l,t+900000);e.on_tick(k.h,t+1800000);e.on_tick(k.c,t+2700000);}
    e.graceful_close(b[hi-1].c,b[hi-1].ts+3600000);
    return {e.total_bp(), e.trades()};
}

int main(){
    std::vector<K> bars;
    glob_t g; glob("data/btc_h1_part*.json",0,nullptr,&g);
    std::vector<std::string> fs(g.gl_pathv,g.gl_pathv+g.gl_pathc); globfree(&g);
    std::sort(fs.begin(),fs.end(),[](const std::string&a,const std::string&b){
        auto nm=[](const std::string&s){size_t p=s.find("part");return atoi(s.c_str()+p+4);}; return nm(a)<nm(b);});
    for(auto&f:fs) parse_file(f.c_str(),bars);
    std::sort(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts<b.ts;});
    printf("loaded %zu H1 bars. D1 engine, fine-fill (H1 stops), 38bp cost.\n\n",bars.size());

    int W=2160; // 90d
    double sumH=0,sumL=0; int trH=0,trL=0, nbull=0; double sumH_bull=0,sumL_bull=0;
    printf("%-12s %-6s | %-10s %-6s | %-10s %-6s\n","date","ret%","HIGH_net","tr","LOW_net","tr");
    for(int s=30*24; s+W<(int)bars.size(); s+=W){
        double r=bars[s+W-1].c/bars[s].c-1.0;
        auto H=run(bars,s,s+W,86400,false);
        auto L=run(bars,s,s+W,86400,true);
        time_t tt=bars[s].ts/1000; char d[16]; strftime(d,sizeof(d),"%Y-%m-%d",gmtime(&tt));
        printf("%-12s %+-6.0f | %+-10.0f %-6d | %+-10.0f %-6d %s\n",d,100*r,H.first,H.second,L.first,L.second,
               (L.first>H.first)?"LOW+":"");
        sumH+=H.first;sumL+=L.first;trH+=H.second;trL+=L.second;
        if(r>0.20){nbull++;sumH_bull+=H.first;sumL_bull+=L.first;}
    }
    printf("\n=== TOTALS over 11yr ===\n");
    printf("HIGH-turnover: net=%+.0fbp  trades=%d\n",sumH,trH);
    printf("LOW-turnover : net=%+.0fbp  trades=%d  (%.0f%% fewer trades)\n",sumL,trL,100.0*(trH-trL)/trH);
    printf("BULL slices only (ret>20%%, n=%d): HIGH=%+.0f  LOW=%+.0f\n",nbull,sumH_bull,sumL_bull);
    printf("\nVERDICT: LOW-turnover %s HIGH-turnover net\n", sumL>sumH?"BEATS":"loses to");
    return 0;
}

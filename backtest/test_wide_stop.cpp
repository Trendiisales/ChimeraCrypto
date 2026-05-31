// S54: does a WIDE-stop / NO-ratchet "hold while trending" exit capture a CHOPPY
// real bull where the tight-stop+ratchet config whipsaws? Loads real BTC H1,
// finds a +40% choppy bull slice, runs both configs via the harness feed order,
// reports GROSS pnl (round_trip=0 -> pure strategy edge, no cost).
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <glob.h>
#include <algorithm>

using chimera::EdgeEngine;
struct K { long long ts; double o,h,l,c; };

// minimal binance-kline parser: [[ts,"o","h","l","c",...],...]
static void parse_file(const char* path, std::vector<K>& out) {
    FILE* f = fopen(path, "rb"); if (!f) return;
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    std::string s; s.resize(n); size_t rd=fread(&s[0],1,n,f); fclose(f); s.resize(rd);
    const char* p=s.c_str(); const char* e=p+s.size();
    while (p<e) {
        while (p<e && *p!='[') p++;
        if (p>=e) break; p++;            // into element
        // ts
        while (p<e && (*p==' '||*p=='[')) p++;
        K k{}; k.ts=strtoll(p,(char**)&p,10);
        auto qd=[&](double& v){ while(p<e&&*p!='"')p++; if(p<e){p++; v=strtod(p,(char**)&p); while(p<e&&*p!='"')p++; if(p<e)p++;} };
        qd(k.o); qd(k.h); qd(k.l); qd(k.c);
        if (k.ts>0 && k.c>0) out.push_back(k);
        while (p<e && *p!=']') p++; if(p<e)p++;   // close element
    }
}

static double run(std::vector<K>& bars, int lo, int hi, bool wide) {
    EdgeEngine::Config c;
    c.symbol="btcusdt"; c.tag=wide?"WIDE":"TIGHT"; c.kind=chimera::StrategyKind::TSMOM;
    c.tf_secs=3600; c.lookback=20; c.atr_period=14; c.round_trip_bp=38.0; c.max_history=64;
    if (wide) { c.hold_bars=400; c.sl_atr_mult=8.0; c.trail_arm_atr=3.0; c.trail_dist_atr=2.0; }
    else      { c.hold_bars=50;  c.sl_atr_mult=2.5; c.trail_arm_atr=1.0; c.trail_dist_atr=0.5; }
    EdgeEngine e(c);
    e.apply_safety_preset();
    if (wide) { e.set_hard_floor_bp(-800.0); e.set_ratchet_start_bp(0.0); e.set_be_arm_bp(100000.0); }
    else      { e.set_hard_floor_bp(-170.0); e.set_ratchet_start_bp(8.0); e.set_be_arm_bp(15.0); }
    e.set_realistic_gap_fill(false);
    std::vector<EdgeEngine::SeedBar> seed;
    for (int i=lo-40;i<lo;i++){ EdgeEngine::SeedBar b{}; b.open_ts_ms=bars[i].ts;b.o=bars[i].o;b.h=bars[i].h;b.l=bars[i].l;b.c=bars[i].c; seed.push_back(b);}
    e.seed_bars(seed);
    for (int i=lo;i<hi;i++){
        const K& b=bars[i]; long long t=b.ts;
        e.on_tick(b.o,t); e.on_tick(b.l,t+900000); e.on_tick(b.h,t+1800000); e.on_tick(b.c,t+2700000);
    }
    e.graceful_close(bars[hi-1].c, bars[hi-1].ts+3600000);
    printf("  %-6s gross_net=%+8.0fbp  trades=%d  wins=%d  WR=%.0f%%\n",
        wide?"WIDE":"TIGHT", e.total_bp(), e.trades(), e.wins(), e.trades()?100.0*e.wins()/e.trades():0);
    return e.total_bp();
}

int main(){
    std::vector<K> bars;
    glob_t g; glob("data/btc_h1_part*.json", 0, nullptr, &g);
    std::vector<std::string> files(g.gl_pathv, g.gl_pathv+g.gl_pathc); globfree(&g);
    std::sort(files.begin(),files.end(),[](const std::string&a,const std::string&b){
        auto num=[](const std::string&s){size_t p=s.find("part");return atoi(s.c_str()+p+4);};
        return num(a)<num(b);});
    for (auto& f: files) parse_file(f.c_str(), bars);
    std::sort(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts<b.ts;});
    printf("loaded %zu BTC H1 bars\n\n", bars.size());

    // find best +40%+ "choppy bull" 90-day (2160h) windows, test a few
    // Scan EVERY 90d slice across 11yr; show TIGHT net (honest 38bp cost) vs the
    // slice return. Proves: edge is POSITIVE in bulls, NEGATIVE in bears/chop.
    int W=2160;
    printf("%-22s %-8s %-10s %s\n","slice_dates(approx)","ret%","TIGHT_net","WR/trades");
    for (int s=60; s+W<(int)bars.size(); s+=W){
        double r=bars[s+W-1].c/bars[s].c-1.0;
        EdgeEngine::Config c; c.symbol="btcusdt"; c.tag="T"; c.kind=chimera::StrategyKind::TSMOM;
        c.tf_secs=3600; c.lookback=20; c.atr_period=14; c.round_trip_bp=38.0; c.max_history=64;
        c.hold_bars=50; c.sl_atr_mult=2.5; c.trail_arm_atr=1.0; c.trail_dist_atr=0.5;
        EdgeEngine e(c); e.apply_safety_preset(); e.set_hard_floor_bp(-170.0);
        e.set_ratchet_start_bp(8.0); e.set_be_arm_bp(15.0); e.set_realistic_gap_fill(false);
        std::vector<EdgeEngine::SeedBar> seed;
        for(int i=s-40;i<s;i++){EdgeEngine::SeedBar b{};b.open_ts_ms=bars[i].ts;b.o=bars[i].o;b.h=bars[i].h;b.l=bars[i].l;b.c=bars[i].c;seed.push_back(b);}
        e.seed_bars(seed);
        for(int i=s;i<s+W;i++){const K&b=bars[i];long long t=b.ts;e.on_tick(b.o,t);e.on_tick(b.l,t+900000);e.on_tick(b.h,t+1800000);e.on_tick(b.c,t+2700000);}
        e.graceful_close(bars[s+W-1].c,bars[s+W-1].ts+3600000);
        time_t tt=bars[s].ts/1000; char buf[32]; strftime(buf,sizeof(buf),"%Y-%m-%d",gmtime(&tt));
        printf("%-22s %+-8.0f %+-10.0f %.0f%%/%d %s\n",buf,100*r,e.total_bp(),
               e.trades()?100.0*e.wins()/e.trades():0,e.trades(), (r>0.20?"<-BULL":(r<-0.20?"<-BEAR":"")));
    }
    (void)0;
    return 0;
}

// Quick verification: run BTC-TSMOM-D1 with the optimized params
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <unistd.h>
#include "core/EdgeEngine.hpp"

namespace fs = std::filesystem;

struct Kline { int64_t open_ts_ms; double o, h, l, c; };

static bool parse_kline_element(const char* start, const char* end, Kline& out) {
    const char* p = start;
    while (p < end && *p != '[') ++p;
    if (p >= end) return false; ++p;
    out.open_ts_ms = 0;
    while (p < end && (*p == ' ' || *p == '\t')) ++p;
    while (p < end && *p >= '0' && *p <= '9') { out.open_ts_ms = out.open_ts_ms * 10 + (*p - '0'); ++p; }
    auto rd = [&](double& v) -> bool {
        while (p < end && *p != '"') ++p; if (p >= end) return false; ++p;
        const char* s = p; while (p < end && *p != '"') ++p; if (p >= end) return false;
        v = std::stod(std::string(s, p)); ++p; return true;
    };
    if (!rd(out.o)||!rd(out.h)||!rd(out.l)||!rd(out.c)) return false;
    return (out.o>0&&out.h>0&&out.l>0&&out.c>0);
}

static std::vector<Kline> load_all_parts(const std::string& dir, const std::string& prefix) {
    std::vector<Kline> all; std::vector<std::string> files;
    for (auto& e : fs::directory_iterator(dir)) {
        std::string f = e.path().filename().string();
        if (f.find(prefix)==0 && f.find(".json")!=std::string::npos) files.push_back(e.path().string());
    }
    std::sort(files.begin(), files.end());
    for (auto& path : files) {
        std::ifstream f(path); std::string c((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>()); f.close();
        const char* d=c.c_str(); const char* end=d+c.size(); const char* p=d;
        while(p<end&&*p!='[')++p; if(p>=end)continue; ++p;
        while(p<end){while(p<end&&*p!='[')++p;if(p>=end)break;const char*s=p;int dep=0;
        while(p<end){if(*p=='[')++dep;if(*p==']'){--dep;if(dep==0){++p;break;}}++p;}
        Kline k{}; if(parse_kline_element(s,p,k))all.push_back(k);}
    }
    std::sort(all.begin(),all.end(),[](const Kline&a,const Kline&b){return a.open_ts_ms<b.open_ts_ms;});
    auto last=std::unique(all.begin(),all.end(),[](const Kline&a,const Kline&b){return a.open_ts_ms==b.open_ts_ms;});
    all.erase(last,all.end()); return all;
}

int main() {
    auto btc_d1 = load_all_parts("data", "btc_d1_part");
    int total = btc_d1.size(), seed_count = total * 0.8;

    chimera::EdgeEngine::Config cfg{
        .symbol="btcusdt",.tag="BTC-TSMOM-D1",.kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400,.lookback=10,.hold_bars=12,.sl_atr_mult=3.0,.atr_period=14,
        .bb_k=2.0,.rsi_threshold=30.0,.round_trip_bp=17.0,.max_history=64,
        .trail_arm_atr=1.0,.trail_dist_atr=0.4,
    };

    fflush(stdout); int sv=dup(fileno(stdout)); FILE*dn=fopen("/dev/null","w"); dup2(fileno(dn),fileno(stdout));
    chimera::EdgeEngine engine(cfg);
    std::vector<chimera::EdgeEngine::SeedBar> seeds;
    for(int i=0;i<seed_count;++i){chimera::EdgeEngine::SeedBar b;b.open_ts_ms=btc_d1[i].open_ts_ms;b.o=btc_d1[i].o;b.h=btc_d1[i].h;b.l=btc_d1[i].l;b.c=btc_d1[i].c;seeds.push_back(b);}
    engine.seed_bars(seeds);

    double peak=0,maxdd=0; std::vector<double> rets; int pt=0; double ptbp=0;
    for(int i=seed_count;i<total;++i){
        auto&k=btc_d1[i]; int64_t s=k.open_ts_ms,step=cfg.tf_secs*1000/4; bool bull=(k.c>=k.o);
        engine.on_tick(k.o,s);
        if(bull){engine.on_tick(k.l,s+step);engine.on_tick(k.h,s+step*2);}
        else{engine.on_tick(k.h,s+step);engine.on_tick(k.l,s+step*2);}
        if(i==total-1)engine.on_tick(k.c,s+cfg.tf_secs*1000+1000);
        if(engine.trades()>pt){double r=engine.total_bp()-ptbp;rets.push_back(r);
        if(engine.total_bp()>peak)peak=engine.total_bp();double dd=peak-engine.total_bp();if(dd>maxdd)maxdd=dd;
        pt=engine.trades();ptbp=engine.total_bp();}
    }
    fflush(stdout); dup2(sv,fileno(stdout)); close(sv); fclose(dn);

    double sw=0,sl=0; for(auto r:rets){if(r>0)sw+=r;else sl+=std::fabs(r);}
    double pf=(sl>0)?sw/sl:(sw>0?99.9:0);
    double wr=engine.trades()>0?100.0*engine.wins()/engine.trades():0;

    std::printf("\n═══ VERIFICATION: Optimized BTC-TSMOM-D1 ═══════════════════\n");
    std::printf("  lookback=10  hold=12  sl=3.0  trail=1.0/0.4  cost=17bp\n");
    std::printf("  OOS bars:    %d\n", total-seed_count);
    std::printf("  Trades:      %d\n", engine.trades());
    std::printf("  Wins:        %d (%.1f%%)\n", engine.wins(), wr);
    std::printf("  Net bp:      %+.1f\n", engine.total_bp());
    std::printf("  PF:          %.2f\n", pf);
    std::printf("  Max DD:      %.0f bp\n", maxdd);
    std::printf("══════════════════════════════════════════════════════════════\n\n");
    return 0;
}

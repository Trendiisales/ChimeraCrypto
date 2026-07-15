// depth_cost_model_bt.cpp — PHASE 2 piece 1 measurement (CORE/MIMIC §2).
// Walks the perp bookDepth ladder for ETH+XRP across every snapshot and reports
// the depth-adjusted exit(sell) / entry(buy) slippage distribution vs campaign
// notional Q. Answers the open design question: is the Phase-1 flat 35bp
// safe_cost optimistic or pessimistic, how does slip scale with size, and is
// XRP materially thinner than ETH (handoff 15m open-Q).
//
// This VALIDATES DepthLiquidationModel before it is wired into core_trigger_bt.
//
// Build: clang++ -std=c++17 -O2 depth_cost_model_bt.cpp -o depth_cost_model_bt
// Data : DC_DEPTH_DIR (default /Users/jo/ChimeraCrypto/data/bookdepth_perp)
// Env  : DC_COINS (default "ETHUSDT,XRPUSDT")
//        DC_NOTIONALS (default "5000,25000,100000,500000,2000000")  USD Q levels
//        DC_FEE_RT (20) known round-trip fee bps ; DC_RESERVE (8) latency+spread+dust reserve

#include "../include/core/DepthLiquidationModel.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

using chimera::DepthBook;
using chimera::DepthSnapshot;

static double envd(const char* k,double d){ const char* v=getenv(k); return v?atof(v):d; }

static std::vector<std::string> csv(const std::string& s){
    std::vector<std::string> v; std::stringstream ss(s); std::string t;
    while(std::getline(ss,t,',')) if(!t.empty()) v.push_back(t); return v; }

static double pct(std::vector<double>& v, double q){
    if(v.empty()) return 0; std::sort(v.begin(),v.end());
    double idx=q*(v.size()-1); size_t lo=(size_t)idx; double f=idx-lo;
    return lo+1<v.size()? v[lo]*(1-f)+v[lo+1]*f : v[lo]; }

int main(){
    std::string dir = getenv("DC_DEPTH_DIR")?getenv("DC_DEPTH_DIR")
                      :"/Users/jo/ChimeraCrypto/data/bookdepth_perp";
    std::vector<std::string> coins = csv(getenv("DC_COINS")?getenv("DC_COINS"):"ETHUSDT,XRPUSDT");
    std::vector<std::string> qs    = csv(getenv("DC_NOTIONALS")?getenv("DC_NOTIONALS")
                                         :"5000,25000,100000,500000,2000000");
    double fee_rt  = envd("DC_FEE_RT",20.0);
    double reserve = envd("DC_RESERVE",8.0);

    std::printf("DEPTH COST MODEL — perp bookDepth walk, exit(sell)+entry(buy) slip vs notional\n");
    std::printf("dir=%s  fee_rt=%.0fbp  reserve(latency+spread+dust)=%.0fbp\n\n", dir.c_str(), fee_rt, reserve);

    for(const auto& coin : coins){
        DepthBook bk;
        size_t n = bk.load_dir(dir, coin);
        if(n==0){ std::printf("%s: NO snapshots (checked %s/%s*)\n\n", coin.c_str(), dir.c_str(), coin.c_str()); continue; }
        // window span
        int64_t t0=bk.snaps().front().ts_ms, t1=bk.snaps().back().ts_ms;
        double days=(t1-t0)/86400000.0;
        // median mid for reference
        std::vector<double> mids; mids.reserve(n);
        for(const auto& s: bk.snaps()) mids.push_back(s.mid);
        double medmid=pct(mids,0.5);
        std::printf("═══ %s ═══  %zu snapshots  %.0f days  median mid≈%.4f\n", coin.c_str(), n, days, medmid);
        std::printf("  %-12s | %-26s | %-26s | %-10s | %s\n",
            "notional$","EXIT sell-slip bps p50/p95/p99","ENTRY buy-slip bps p50/p95/p99",
            "beyond%","safe_cost_bps (fee+entry_p99+exit_p99+reserve)");
        for(const auto& qstr : qs){
            double Qusd=atof(qstr.c_str());
            std::vector<double> ex, en; int beyond=0;
            for(const auto& s: bk.snaps()){
                if(!s.valid||s.mid<=0) continue;
                double Qbase=Qusd/s.mid;
                bool bx=false,bn=false;
                ex.push_back(s.sell_slip_bps(Qbase,&bx));
                en.push_back(s.buy_slip_bps(Qbase,&bn));
                if(bx||bn) beyond++;
            }
            double ex50=pct(ex,0.5),ex95=pct(ex,0.95),ex99=pct(ex,0.99);
            double en50=pct(en,0.5),en95=pct(en,0.95),en99=pct(en,0.99);
            double safe = fee_rt + en99 + ex99 + reserve;
            char qh[16]; std::snprintf(qh,sizeof(qh), Qusd>=1e6?"%.1fM":"%.0fk", Qusd>=1e6?Qusd/1e6:Qusd/1e3);
            std::printf("  %-12s | %6.1f /%6.1f /%6.1f       | %6.1f /%6.1f /%6.1f       | %6.1f%%   | %.1f\n",
                qh, ex50,ex95,ex99, en50,en95,en99, ex.empty()?0:100.0*beyond/ex.size(), safe);
        }
        std::printf("\n");
    }
    std::printf("READ: safe_cost_bps here = fee_rt + p99 entry buy-slip + p99 exit sell-slip + reserve.\n");
    std::printf("Compare vs Phase-1 flat 35bp. <35 => flat was pessimistic (edge survives easier);\n");
    std::printf(">35 (thin coin / large Q) => flat was optimistic (re-validate harder). beyond%%>0 => Q\n");
    std::printf("exceeds the +-5%% book at that snapshot (slip is a floored estimate there).\n");
    return 0;
}

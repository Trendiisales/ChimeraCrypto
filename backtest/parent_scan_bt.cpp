// ============================================================================
// parent_scan_bt.cpp — PHASE 1 crypto trend-PARENT viability scan.
//
// Drives the REAL chimera::EdgeEngine (include/core/EdgeEngine.hpp) so results
// reflect live behaviour, over a single CERTIFIED-CLEAN daily-bar CSV slice.
//
// Purpose: determine which trend kind (TSMOM / SUPERTREND / KELTNER_BREAK /
// REGIME_SWITCH) is a viable PARENT for a mimic-after-BE — i.e. spends enough
// held time PAST break-even that a companion armed at BE has something to ride.
//
// Faithful replay (mirrors backtest/backtest_harness.cpp run_backtest):
//   - seed first `seed` bars (in-window warmup; indicators warm inside the SAME
//     certified file — no cross-slice / uncertified data used)
//   - feed remaining bars as ticks in O, LOW(adverse-first), HIGH order — the
//     honest long-only fill (SL tested before TP on every bar)
//   - force-close the final bar
//   - net_bp = gross - round_trip_bp is charged inside EdgeEngine::exit_position_
//
// Instruments, over the SCORED (post-seed) region:
//   trades, wins, net_bp, PF, maxDD_bp, worst_trade_bp,
//   bars_in_position, bars_past_BE (unrealised_bp > round_trip_bp),
//   %-time-past-BE  <-- the key mimic metric
//   + a random-entry control (same trade count & mean hold) for a beta check.
//
// Build: cd backtest && g++ -std=c++20 -O2 -I../include parent_scan_bt.cpp -o parent_scan_bt
// Usage: ./parent_scan_bt <slice.csv> <SYMBOL> <KIND> <cost_bp> <seed_bars>
//   KIND in {TSMOM,SUPERTREND,KELTNER_BREAK,REGIME_SWITCH}
// Emits ONE CSV line to stdout:
//   sym,kind,cost,scored,trades,wins,net_bp,pf,maxdd_bp,worst_bp,inpos,pastbe,pct_pastbe,rand_mean_bp,rand_pctile
// ============================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>
#include <unistd.h>
#include <fcntl.h>
#include "core/EdgeEngine.hpp"

struct Bar { int64_t ts_ms; double o,h,l,c; };

// xsec_seed daily format: openTime,open,high,low,close,volume  (header line)
static std::vector<Bar> load_csv(const char* path){
    std::vector<Bar> out;
    std::ifstream f(path);
    if(!f){ std::fprintf(stderr,"cannot open %s\n",path); return out; }
    std::string line; bool first=true;
    while(std::getline(f,line)){
        if(line.empty()) continue;
        if(first){ first=false; if(!isdigit((unsigned char)line[0])) continue; }
        // parse csv
        std::vector<std::string> p; std::string cur; std::stringstream ss(line);
        while(std::getline(ss,cur,',')) p.push_back(cur);
        if(p.size()<5) continue;
        Bar b;
        b.ts_ms=std::strtoll(p[0].c_str(),nullptr,10);
        b.o=std::strtod(p[1].c_str(),nullptr);
        b.h=std::strtod(p[2].c_str(),nullptr);
        b.l=std::strtod(p[3].c_str(),nullptr);
        b.c=std::strtod(p[4].c_str(),nullptr);
        if(b.o>0&&b.h>0&&b.l>0&&b.c>0) out.push_back(b);
    }
    return out;
}

int main(int argc,char**argv){
    if(argc<6){ std::fprintf(stderr,"usage: %s <csv> <SYM> <KIND> <cost_bp> <seed_bars>\n",argv[0]); return 2; }
    const char* csv=argv[1];
    std::string sym=argv[2];
    std::string kindstr=argv[3];
    double cost=std::atof(argv[4]);
    int seed=std::atoi(argv[5]);
    std::string preset = (argc>6) ? argv[6] : "live";  // "live" | "legacy"

    chimera::StrategyKind kind;
    bool ride=false; int atrp=14;
    if(kindstr=="TSMOM"){ kind=chimera::StrategyKind::TSMOM; ride=false; atrp=14; }
    else if(kindstr=="SUPERTREND"){ kind=chimera::StrategyKind::SUPERTREND; ride=false; atrp=14; }
    else if(kindstr=="KELTNER_BREAK"){ kind=chimera::StrategyKind::KELTNER_BREAK; ride=true; atrp=20; }
    else if(kindstr=="REGIME_SWITCH"){ kind=chimera::StrategyKind::REGIME_SWITCH; ride=true; atrp=14; }
    else { std::fprintf(stderr,"bad KIND %s\n",kindstr.c_str()); return 2; }

    auto bars=load_csv(csv);
    if((int)bars.size()<seed+30){
        // not enough scored data
        std::printf("%s,%s,%.0f,0,0,0,0,0,0,0,0,0,0,0,0\n",sym.c_str(),kindstr.c_str(),cost);
        return 0;
    }

    // ── Live D1 config (matches src/main.cpp folded configs) ──────────────
    // lookback=20 hold=12 sl=3.0*atr rt=cost trail_arm=1.0 dist=0.4 tighten=3.0/0.25
    // Non-designated fields keep EdgeEngine struct defaults (= live: hard_floor
    // -100, ratchet_start 15, be_arm 50, giveback 100/30%, early_kill -50@mfe10).
    std::string low=sym; for(auto&ch:low) ch=(char)tolower((unsigned char)ch);
    chimera::EdgeEngine::Config cfg{};
    cfg.symbol=low; cfg.tag=sym+"-"+kindstr; cfg.kind=kind;
    cfg.tf_secs=86400; cfg.lookback=20; cfg.hold_bars=12; cfg.sl_atr_mult=3.0;
    cfg.atr_period=atrp; cfg.ride_to_flip=ride;
    cfg.keltner_ema_len=20; cfg.keltner_atr_mult=2.0;
    cfg.st_multiplier=3.0; cfg.st_atr_period=10;
    cfg.round_trip_bp=cost; cfg.max_history=64;
    // Baseline edge measure on OHLC bars: fill stops AT the stop level (liquid
    // continuous fill), NOT at the coarse-bar penetrating low. Per EdgeEngine
    // check_exits_ note — realistic_gap_fill(default true) is a LIVE dense-tick
    // setting; on O/H/L/C replay it turns every daily low into a fake gap-through
    // and wildly overstates trend-engine losses. Off = documented baseline.
    cfg.realistic_gap_fill=false;
    cfg.trail_arm_atr=1.0; cfg.trail_dist_atr=0.4;
    cfg.trail_tighten_atr=3.0; cfg.trail_tighten_dist_atr=0.25;

    // ── LIVE-FAITHFUL EXIT PRESET (S36 protection_only) ────────────────────
    // main.cpp applies the staged-ratchet-ONLY preset to live EdgeEngines; the
    // raw Config defaults (early_kill -50@mfe10, hard_floor -100, giveback
    // 100/30%) were proven CATASTROPHIC on TSMOM (harness S36 matrix:
    // prod_tiered -753,182bp / 0-of-15 profitable vs staged_only +274,840bp /
    // 15-of-15). Disable destructive layers; keep staged BE-ratchet + bespoke
    // trail. (ride_to_flip kinds ignore all of this — check_exits_ early-returns.)
    cfg.hard_floor_bp=0.0; cfg.early_kill_bp=0.0; cfg.early_kill_mfe=0.0;
    cfg.early_kill_min_hold_ms=0; cfg.giveback_arm_bp=0.0; cfg.signal_confirm_bars=1;
    cfg.ratchet_start_bp=cost; cfg.be_arm_bp=cost+10.0; cfg.ratchet_lock_pct=0.75;
    cfg.prog_lock_pct_2=0.85; cfg.prog_lock_pct_3=0.90; cfg.prog_lock_pct_4=0.95;

    // Fairness alt for the trend-with-stop kinds: pure ATR trail + hard SL +
    // time exit, NO staged ratchet / tighten (disable_s32_protections). Ensures
    // we don't strawman-kill TSMOM/SUPERTREND with a pathological tight preset.
    if(preset=="legacy"){
        cfg.ratchet_start_bp=0.0; cfg.be_arm_bp=0.0; cfg.ratchet_lock_pct=0.0;
        cfg.prog_lock_pct_2=0.0; cfg.prog_lock_pct_3=0.0; cfg.prog_lock_pct_4=0.0;
        cfg.trail_tighten_atr=0.0; cfg.trail_arm_atr=1.0; cfg.trail_dist_atr=0.5;
    }

    // redirect fd 1 -> /dev/null for the whole engine lifetime (suppress its
    // ARMED/SEED/trade prints); restored before we emit the CSV line.
    fflush(stdout);
    int saved_fd=dup(1);
    { int devnull=open("/dev/null",O_WRONLY); dup2(devnull,1); close(devnull); }

    chimera::EdgeEngine eng(cfg);

    // Collect trades
    struct TR{ double net_bp; };
    std::vector<double> trade_bps;
    eng.set_on_trade([&](const chimera::EdgeEngine::TradeRecord& r){
        trade_bps.push_back(r.total_net_bp);
    });

    // Per-bar past-BE tracking via on_bar callback (fires inside close_bar_,
    // position state is current, br.c is the just-closed bar close).
    long bars_in_pos=0, bars_past_be=0;
    // capture engine ptr for unrealised
    chimera::EdgeEngine* ep=&eng;
    eng.set_on_bar([&](const chimera::EdgeEngine::BarRecord& br){
        if(br.in_position){
            bars_in_pos++;
            double ub=ep->unrealised_bp(br.c);
            if(ub>cost) bars_past_be++;
        }
    });

    // Seed
    std::vector<chimera::EdgeEngine::SeedBar> seeds; seeds.reserve(seed);
    for(int i=0;i<seed;i++){ chimera::EdgeEngine::SeedBar sb; sb.open_ts_ms=bars[i].ts_ms; sb.o=bars[i].o; sb.h=bars[i].h; sb.l=bars[i].l; sb.c=bars[i].c; seeds.push_back(sb); }
    eng.seed_bars(seeds);

    int total=(int)bars.size();
    int scored=total-seed;
    std::vector<double> scored_close;  // for random control
    for(int i=seed;i<total;i++){
        const Bar& k=bars[i];
        int64_t t0=k.ts_ms;
        int64_t step=(cfg.tf_secs*1000)/4;
        eng.on_tick(k.o,t0);
        eng.on_tick(k.l,t0+step);     // adverse first
        eng.on_tick(k.h,t0+step*2);   // favourable second
        if(i==total-1){ eng.on_tick(k.c,t0+cfg.tf_secs*1000+1000); }
        scored_close.push_back(k.c);
    }
    // graceful close any open trade at final close (so it's counted)
    eng.graceful_close(bars[total-1].c, bars[total-1].ts_ms+cfg.tf_secs*1000+2000);

    // restore stdout
    fflush(stdout);
    dup2(saved_fd,1); close(saved_fd);

    // Compute stats
    int trades=(int)trade_bps.size();
    int wins=0; double net=0,sumw=0,suml=0,worst=0;
    double eq=0,peak=0,maxdd=0;
    for(double b:trade_bps){
        net+=b; if(b>0){wins++;sumw+=b;} else suml+=std::fabs(b);
        if(b<worst) worst=b;
        eq+=b; if(eq>peak)peak=eq; double dd=peak-eq; if(dd>maxdd)maxdd=dd;
    }
    double pf = suml>0 ? sumw/suml : (sumw>0?99.9:0.0);
    double pct_be = bars_in_pos>0 ? 100.0*bars_past_be/bars_in_pos : 0.0;

    // ── Random-entry control ──────────────────────────────────────────────
    // Same trade count N and mean hold H; 500 samples of N random long holds
    // over the scored region, net of cost. Report mean sample-net and the
    // percentile rank of the engine's net within the sample distribution.
    double rand_mean=0; double rand_pctile=-1;
    int H = (trades>0 && bars_in_pos>0) ? std::max(1,(int)std::llround((double)bars_in_pos/trades)) : 0;
    if(trades>0 && H>0 && (int)scored_close.size()>H+2){
        std::mt19937 rng(12345);
        int SN=(int)scored_close.size();
        std::uniform_int_distribution<int> pick(0, SN-1-H);
        const int SAMP=500;
        std::vector<double> sums; sums.reserve(SAMP);
        double acc=0;
        for(int s=0;s<SAMP;s++){
            double tot=0;
            for(int t=0;t<trades;t++){
                int e=pick(rng);
                double ret=(scored_close[e+H]/scored_close[e]-1.0)*1e4 - cost;
                tot+=ret;
            }
            sums.push_back(tot); acc+=tot;
        }
        rand_mean=acc/SAMP;
        std::sort(sums.begin(),sums.end());
        int below=(int)(std::lower_bound(sums.begin(),sums.end(),net)-sums.begin());
        rand_pctile=100.0*below/SAMP;
    }

    // Emit CSV (fd 1 may be /dev/tty now; also print to fd via dprintf to be safe)
    char outbuf[512];
    std::snprintf(outbuf,sizeof(outbuf),
        "%s,%s,%.0f,%d,%d,%d,%.1f,%.3f,%.1f,%.1f,%ld,%ld,%.1f,%.1f,%.1f,%.1f,%.1f\n",
        sym.c_str(),kindstr.c_str(),cost,scored,trades,wins,net,pf,maxdd,worst,
        bars_in_pos,bars_past_be,pct_be,rand_mean,rand_pctile,sumw,suml);
    ::fputs(outbuf,stdout); fflush(stdout);
    return 0;
}

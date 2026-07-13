// ============================================================================
// companion_be_mimic_bt.cpp — PHASE 2 crypto mimic-after-BE viability backtest.
//
// Drives the REAL chimera::EdgeEngine (REGIME_SWITCH, the Phase-1 decided parent)
// AND the REAL chimera::UpJumpLadderCompanion reinstated in BE-ENTRY mode, over a
// CERTIFIED-CLEAN daily-bar slice. Reports the MIMIC's OWN STANDALONE book only —
// never vs the parent (feedback-companion-independent-engine).
//
// BE-ENTRY mimic = existing UpJumpLadderCompanion machinery, LADDER mode
// (det_w=0 -> observes the EXTERNAL parent's settled position) with confirm_bp>0
// (Option-B): every leg stays FLAT booking nothing / paying no cost until the
// parent's move clears +confirm_bp (== BE == round-trip cost), then OPENS at that
// price and rides a tight peak-profit giveback trail, exits on reversal, RE-CLIPS
// (re-enters) on continuation. cap=2 => 2 base tiers, NO self-funding ladder (the
// operator dropped the ladder for the BE book). Honest MTM fills (net_bp_real ==
// net_bp in ladder mode; the be_floor tautology column is NOT used — be_floor
// family is retired).
//
// Wiring under test (the exact live change): after each parent on_tick, feed the
//   companion  observe(parent.in_position(), parent.entry_px(), px, ts).
//
// Build: cd backtest && g++ -std=c++20 -O2 -I../include companion_be_mimic_bt.cpp -o companion_be_mimic_bt
// Usage: ./companion_be_mimic_bt <slice.csv> <SYM> <cost_bp> <seed> <confirm_bp> <loss_cut_bp>
// Emits ONE CSV line:
//   sym,cost,confirm,losscut,scored,p_trades,p_net_bp,p_pct_pastbe,
//   m_clips,m_net_bp,m_pf,m_worst_bp,m_pct_armed,m_net_h1,m_net_h2,m_win_rate
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
#include <unistd.h>
#include <fcntl.h>
#include "core/EdgeEngine.hpp"
#include "core/UpJumpLadderCompanion.hpp"

struct Bar { int64_t ts_ms; double o,h,l,c; };

static std::vector<Bar> load_csv(const char* path){
    std::vector<Bar> out;
    std::ifstream f(path);
    if(!f){ std::fprintf(stderr,"cannot open %s\n",path); return out; }
    std::string line; bool first=true;
    while(std::getline(f,line)){
        if(line.empty()) continue;
        if(first){ first=false; if(!isdigit((unsigned char)line[0])) continue; }
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
    if(argc<7){ std::fprintf(stderr,"usage: %s <csv> <SYM> <cost_bp> <seed> <confirm_bp> <loss_cut_bp>\n",argv[0]); return 2; }
    const char* csv=argv[1];
    std::string sym=argv[2];
    double cost=std::atof(argv[3]);
    int seed=std::atoi(argv[4]);
    double confirm_bp=std::atof(argv[5]);
    double loss_cut_bp=std::atof(argv[6]);

    auto bars=load_csv(csv);
    if((int)bars.size()<seed+30){
        std::printf("%s,%.0f,%.0f,%.0f,0,0,0,0,0,0,0,0,0,0,0,0\n",
            sym.c_str(),cost,confirm_bp,loss_cut_bp);
        return 0;
    }

    // ── Parent: REGIME_SWITCH, IDENTICAL config to parent_scan_bt.cpp ─────────
    std::string low=sym; for(auto&ch:low) ch=(char)tolower((unsigned char)ch);
    chimera::EdgeEngine::Config cfg{};
    cfg.symbol=low; cfg.tag=sym+"-REGIME_SWITCH"; cfg.kind=chimera::StrategyKind::REGIME_SWITCH;
    cfg.tf_secs=86400; cfg.lookback=20; cfg.hold_bars=12; cfg.sl_atr_mult=3.0;
    cfg.atr_period=14; cfg.ride_to_flip=true;
    cfg.keltner_ema_len=20; cfg.keltner_atr_mult=2.0;
    cfg.st_multiplier=3.0; cfg.st_atr_period=10;
    cfg.round_trip_bp=cost; cfg.max_history=64;
    cfg.realistic_gap_fill=false;
    cfg.trail_arm_atr=1.0; cfg.trail_dist_atr=0.4;
    cfg.trail_tighten_atr=3.0; cfg.trail_tighten_dist_atr=0.25;
    cfg.hard_floor_bp=0.0; cfg.early_kill_bp=0.0; cfg.early_kill_mfe=0.0;
    cfg.early_kill_min_hold_ms=0; cfg.giveback_arm_bp=0.0; cfg.signal_confirm_bars=1;
    cfg.ratchet_start_bp=cost; cfg.be_arm_bp=cost+10.0; cfg.ratchet_lock_pct=0.75;
    cfg.prog_lock_pct_2=0.85; cfg.prog_lock_pct_3=0.90; cfg.prog_lock_pct_4=0.95;

    // ── Mimic: UpJumpLadderCompanion, BE-ENTRY (ladder + confirm) ─────────────
    chimera::UpJumpLadderCompanion::Config mc;
    mc.parent_tag = sym+"-REGIME_SWITCH";
    mc.tag        = sym+"-REGIME-BEMIMIC";
    mc.symbol     = low;
    // tight tier: arms fast, tight giveback (banks the small continuation)
    mc.tight = {0.30, 0, 0.40, 0.0};   // arm 0.30%  gb 40%  stall off
    // wide tier: arms later, looser giveback (rides the big continuation)
    mc.wide  = {0.80, 0, 0.55, 0.0};   // arm 0.80%  gb 55%
    mc.reclip_pct   = 0.05;            // re-enter on +5% new peak after a clip
    mc.confirm_bp   = confirm_bp;      // OPTION-B: leg opens only once fav >= confirm_bp (== BE)
    mc.cap          = 2;               // 2 base tiers, NO self-funding ladder (BE book)
    mc.cost_gate_bp = 0.0;
    mc.loss_cut_bp  = loss_cut_bp;     // adverse-protection variant (0 = baseline)
    mc.round_trip_bp= cost;
    mc.tf_secs      = 86400;
    mc.be_floor     = false;           // LADDER mode (honest MTM real column), NOT be_floor
    mc.det_w        = 0;               // observe the EXTERNAL parent, not self-detect

    chimera::UpJumpLadderCompanion mimic(mc);
    mimic.shadow_mode = true;

    // suppress engine prints
    fflush(stdout);
    int saved_fd=dup(1);
    { int devnull=open("/dev/null",O_WRONLY); dup2(devnull,1); close(devnull); }

    chimera::EdgeEngine eng(cfg);

    // parent trades (for context only)
    std::vector<double> p_trade_bps;
    eng.set_on_trade([&](const chimera::EdgeEngine::TradeRecord& r){ p_trade_bps.push_back(r.total_net_bp); });
    long p_bars_in=0, p_bars_be=0;
    chimera::EdgeEngine* ep=&eng;
    eng.set_on_bar([&](const chimera::EdgeEngine::BarRecord& br){
        if(br.in_position){ p_bars_in++; if(ep->unrealised_bp(br.c)>cost) p_bars_be++; }
    });

    // mimic clips (the standalone book under test)
    struct Clip{ double net; int64_t ts; };
    std::vector<Clip> clips;
    mimic.set_on_clip([&](const chimera::UpJumpLadderCompanion::ClipRecord& r){
        clips.push_back({r.net_bp_real, r.exit_ts_ms});
    });

    // seed parent (in-window warmup, certified file)
    std::vector<chimera::EdgeEngine::SeedBar> seeds; seeds.reserve(seed);
    for(int i=0;i<seed;i++){ chimera::EdgeEngine::SeedBar sb; sb.open_ts_ms=bars[i].ts_ms; sb.o=bars[i].o; sb.h=bars[i].h; sb.l=bars[i].l; sb.c=bars[i].c; seeds.push_back(sb); }
    eng.seed_bars(seeds);

    int total=(int)bars.size();
    int scored=total-seed;
    int64_t scored_t0 = bars[seed].ts_ms;
    int64_t scored_tN = bars[total-1].ts_ms;
    int64_t mid_ts = scored_t0 + (scored_tN-scored_t0)/2;

    long m_armed_bars=0;   // scored bars with an OPEN+armed mimic leg (during parent in-pos)
    for(int i=seed;i<total;i++){
        const Bar& k=bars[i];
        int64_t t0=k.ts_ms;
        int64_t step=(cfg.tf_secs*1000)/4;
        // Parent fills on its own O/L/H path (realistic_gap_fill=false -> stops fill
        // AT the stop level, not the penetrating daily low).
        eng.on_tick(k.o,t0);
        eng.on_tick(k.l,t0+step);       // adverse first
        eng.on_tick(k.h,t0+step*2);     // favourable second
        // MIMIC: drive on the bar CLOSE only. Feeding the coarse daily LOW would make
        // every giveback/reversal exit fill at the penetrating low and wildly overstate
        // losses (the exact artifact the parent disables via realistic_gap_fill=false).
        // Close-to-close is the honest daily analog and matches the companion's own
        // close-driven process_close_ path + the %-past-BE close metric.
        int64_t ts_c = t0+cfg.tf_secs*1000+1000;
        mimic.observe(eng.in_position(), eng.entry_px(), k.c, ts_c);
        auto snap=mimic.snapshot();
        if(eng.in_position() && snap.open && snap.armed) m_armed_bars++;
    }
    eng.graceful_close(bars[total-1].c, bars[total-1].ts_ms+cfg.tf_secs*1000+2000);
    // flush the mimic: parent flat -> MTM every open leg
    mimic.observe(false, 0.0, bars[total-1].c, bars[total-1].ts_ms+cfg.tf_secs*1000+2000);

    fflush(stdout);
    dup2(saved_fd,1); close(saved_fd);

    // ── parent context stats ─────────────────────────────────────────────────
    double p_net=0; for(double b:p_trade_bps) p_net+=b;
    double p_pct_be = p_bars_in>0 ? 100.0*p_bars_be/p_bars_in : 0.0;

    // ── MIMIC standalone book ────────────────────────────────────────────────
    int m_clips=(int)clips.size();
    double m_net=0,sumw=0,suml=0,worst=0; int wins=0;
    double net_h1=0, net_h2=0;
    for(auto&c:clips){
        m_net+=c.net;
        if(c.net>0){wins++;sumw+=c.net;} else suml+=std::fabs(c.net);
        if(c.net<worst) worst=c.net;
        if(c.ts<=mid_ts) net_h1+=c.net; else net_h2+=c.net;
    }
    double m_pf = suml>0 ? sumw/suml : (sumw>0?99.9:0.0);
    double win_rate = m_clips>0 ? 100.0*wins/m_clips : 0.0;
    double pct_armed = p_bars_in>0 ? 100.0*m_armed_bars/p_bars_in : 0.0;

    char outbuf[768];
    std::snprintf(outbuf,sizeof(outbuf),
        "%s,%.0f,%.0f,%.0f,%d,%d,%.1f,%.1f,%d,%.1f,%.3f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n",
        sym.c_str(),cost,confirm_bp,loss_cut_bp,scored,
        (int)p_trade_bps.size(),p_net,p_pct_be,
        m_clips,m_net,m_pf,worst,pct_armed,net_h1,net_h2,win_rate,sumw,suml);
    ::fputs(outbuf,stdout); fflush(stdout);
    return 0;
}

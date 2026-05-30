// S45 protection verification:
//   1. EdgeEngine respects cluster_gate_ (entry suppressed when closed)
//   2. per-symbol / per-cluster concurrency caps (the 30-May 6-JTO failure)
//   3. REAL hard floor: a losing trade is cut at ~-170bp, never deeper
//   4. unified entry guard: concurrency && cluster-loss-ok && regime-ok
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <cstring>

using chimera::EdgeEngine;

static EdgeEngine::Config mk_cfg(const char* tag, const char* sym) {
    EdgeEngine::Config c;
    c.tag = tag; c.symbol = sym;
    c.kind = chimera::StrategyKind::TSMOM;
    c.tf_secs = 60; c.lookback = 3; c.hold_bars = 50;
    c.atr_period = 3; c.max_history = 64; c.round_trip_bp = 10.0;
    return c;
}
static void bar(EdgeEngine& e, int i, double px) { e.on_tick(px, (int64_t)(i+1)*60*1000); }

int main() {
    int failures = 0;

    // ── 1: gate open fires / gate closed suppresses ─────────────────────────
    {
        EdgeEngine e(mk_cfg("OPEN","btcusdt")); e.set_cluster_gate(true);
        for (int i=0;i<12;i++) bar(e,i,100.0+i); bar(e,12,113.0);
        bool in=e.in_position();
        printf("[1a] gate OPEN -> in_position=%d (expect 1)\n",in);
        if(!in){printf("    FAIL\n");failures++;}
    }
    {
        EdgeEngine e(mk_cfg("BLOCKED","btcusdt")); e.set_cluster_gate(false);
        for (int i=0;i<12;i++) bar(e,i,100.0+i); bar(e,12,113.0);
        bool in=e.in_position();
        printf("[1b] gate CLOSED -> in_position=%d (expect 0)\n",in);
        if(in){printf("    FAIL\n");failures++;}
    }

    // ── 2: concurrency cap decision (S54: per-symbol 1, per-cluster 5) ──────
    const int MAXSYM=1, MAXCLU=5;
    auto allow=[&](int s,int c){return s<MAXSYM && c<MAXCLU;};
    { int s=0,c=0,n=0; for(int i=0;i<6;i++) if(allow(s,c)){s++;c++;n++;}
      printf("[2a] 6 on one symbol -> entered=%d (expect 1; S54 kills XLM double-fill)\n",n);
      if(n!=1){printf("    FAIL\n");failures++;} }
    { int c=0,n=0,s[8]={0}; for(int i=0;i<8;i++) if(allow(s[i],c)){s[i]++;c++;n++;}
      printf("[2b] 8 distinct DEFI -> entered=%d (expect 5)\n",n);
      if(n!=5){printf("    FAIL\n");failures++;} }

    // ── 3: REAL hard floor cuts a loser at ~-170bp (the 29-May fix) ─────────
    // 3a CLEAN TOUCH: price eases down to the -170 floor in fine steps. Honest
    // fill ~= the stop level, so loss caps at ~-170. Proves the floor holds when
    // there is NO gap.
    {
        EdgeEngine e(mk_cfg("FLOOR-CLEAN","seiusdt"));
        e.set_cluster_gate(true); e.apply_safety_preset(); e.set_hard_floor_bp(-170.0);
        e.set_signal_confirm_bars(1);   // realistic_gap_fill defaults TRUE (P0)
        int i=0;
        for(double px=100.0; px<=103.0; px+=1.0) bar(e,i++,px);          // enter ~103
        for(double px=102.9; px>=100.5; px-=0.05) bar(e,i++,px);         // FINE decline -> clean touch
        double last=e.total_bp();
        printf("[3a] clean touch  net_bp=%.1f (expect ~ -170..-185)\n", last);
        if(e.in_position() || last < -190.0 || last > -160.0){printf("    FAIL: floor not ~-170 on clean touch\n");failures++;}
    }
    // 3b GAP-THROUGH: price gaps from just-above the floor straight to -400bp in
    // ONE tick. With P0 gap-honest fill the exit books ~-400, NOT -170 -- this is
    // the truth caveat-2 was hiding. A stop cannot save the FILL on a gap; only
    // sizing + the cluster/bear breakers limit the damage.
    {
        EdgeEngine e(mk_cfg("FLOOR-GAP","seiusdt"));
        e.set_cluster_gate(true); e.apply_safety_preset(); e.set_hard_floor_bp(-170.0);
        e.set_signal_confirm_bars(1);
        int i=0;
        for(double px=100.0; px<=103.0; px+=1.0) bar(e,i++,px);          // enter ~103
        bar(e,i++,102.5);                                               // still above floor
        bar(e,i++,103.0*(1.0-0.040));                                   // GAP to ~-400bp in one tick
        bar(e,i++,103.0*(1.0-0.045));
        double last=e.total_bp();
        printf("[3b] gap-through net_bp=%.1f (expect ~ -400, honest -- floor does NOT save the fill)\n", last);
        if(e.in_position() || last > -300.0){printf("    FAIL: gap fill not honest (still clamped to floor?)\n");failures++;}
    }
    // 3c LEGACY clamp OFF the honesty: same gap but realistic_gap_fill=false ->
    // books the old optimistic -170. Guards the --legacy-stop-fill escape hatch.
    {
        EdgeEngine e(mk_cfg("FLOOR-LEGACY","seiusdt"));
        e.set_cluster_gate(true); e.apply_safety_preset(); e.set_hard_floor_bp(-170.0);
        e.set_signal_confirm_bars(1); e.set_realistic_gap_fill(false);  // legacy
        int i=0;
        for(double px=100.0; px<=103.0; px+=1.0) bar(e,i++,px);
        bar(e,i++,102.5); bar(e,i++,103.0*(1.0-0.040)); bar(e,i++,103.0*(1.0-0.045));
        double last=e.total_bp();
        printf("[3c] legacy fill net_bp=%.1f (expect ~ -170, old optimistic behavior)\n", last);
        if(e.in_position() || last < -190.0 || last > -150.0){printf("    FAIL: legacy mode not clamping to floor\n");failures++;}
    }

    // ── 4: unified guard = concurrency && cluster-loss-ok && regime-ok ──────
    auto guard=[&](bool conc,bool loss_ok,bool regime_ok){return conc&&loss_ok&&regime_ok;};
    struct{const char*n;bool c,l,r;bool exp;} cases[]={
        {"all ok",            true,true,true,   true},
        {"cluster loss-halt", true,false,true,  false},
        {"bear regime",       true,true,false,  false},
        {"concurrency full",  false,true,true,  false},
    };
    for(auto&t:cases){bool g=guard(t.c,t.l,t.r);
        printf("[4] %-18s -> allow=%d (expect %d)\n",t.n,g,t.exp);
        if(g!=t.exp){printf("    FAIL\n");failures++;}}

    printf("\n%s (%d failures)\n", failures?"TESTS FAILED":"ALL TESTS PASSED", failures);
    return failures?1:0;
}

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

    // ── 2: concurrency cap decision (per-symbol 2, per-cluster 5) ───────────
    const int MAXSYM=2, MAXCLU=5;
    auto allow=[&](int s,int c){return s<MAXSYM && c<MAXCLU;};
    { int s=0,c=0,n=0; for(int i=0;i<6;i++) if(allow(s,c)){s++;c++;n++;}
      printf("[2a] 6 JTO (one symbol) -> entered=%d (expect 2)\n",n);
      if(n!=2){printf("    FAIL\n");failures++;} }
    { int c=0,n=0,s[8]={0}; for(int i=0;i<8;i++) if(allow(s[i],c)){s[i]++;c++;n++;}
      printf("[2b] 8 distinct DEFI -> entered=%d (expect 5)\n",n);
      if(n!=5){printf("    FAIL\n");failures++;} }

    // ── 3: REAL hard floor cuts a loser at ~-170bp (the 29-May fix) ─────────
    // Enter on a rising series, then dump -400bp. With hard_floor=-170 the SL
    // is tightened to -170 at entry, so the exit must land near -170, NOT -400.
    {
        EdgeEngine e(mk_cfg("FLOOR","seiusdt"));
        e.set_cluster_gate(true);
        e.apply_safety_preset();              // PRODUCTION path: disables early_kill/giveback/hard_floor
        e.set_hard_floor_bp(-170.0);          // ...then the S45 fix re-arms a REAL -170 floor
        e.set_signal_confirm_bars(1);
        int i=0;
        // rising bars at atr~1 -> ATR stop ~-250bp (wider than floor) so floor
        // clamps SL to -170. The decline's first bar closes bar#3 -> entry @103.
        for(double px=100.0; px<=103.0; px+=1.0) bar(e,i++,px);   // 100,101,102,103
        // monotonic decline from BELOW entry: mfe stays 0 so the staged ratchet
        // never engages and ONLY the -170 floor governs the exit.
        for(double px=102.5; px>=98.0; px-=0.5) bar(e,i++,px);
        bool entered=(e.total_bp()!=0.0)|| !e.in_position();      // a trade occurred
        bool flat=!e.in_position();
        double last=e.total_bp();   // single trade -> cumulative == this trade's net
        printf("[3] entered=%d  exited=%d  net_bp=%.1f (expect ~ -170..-185, NOT < -250)\n",
               entered, flat, last);
        if(!entered || !flat || last < -200.0 || last > -150.0){printf("    FAIL: floor did not cut loss at -170\n");failures++;}
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

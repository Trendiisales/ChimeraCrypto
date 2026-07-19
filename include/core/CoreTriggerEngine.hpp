// CoreTriggerEngine.hpp — LIVE streaming mirror of run_core (core_trigger_p2_bt.cpp)
// =================================================================================
// ADVERSE-PROTECTION: trend-ride exit — backtested. Verdict: NO cold loss-cut. The CORE
//   edge is a wide trend-ride (structural stop at the pullback higher-low + adaptive ATR
//   trail + VWAP-loss exit). Tightening a fixed cold-cut LOWERS net (per the 2026-06-17
//   swing-protection sweep + the CORE Phase-1/2 trail sensitivity: trail 200-280 plateau,
//   a tight trail collapses the edge). In-flight risk is bounded by the structural stop
//   (pb_low * (1 - stopbuf)). Faithful BT: ETH n13 +2120 PF3.71 worst -245bp, XRP n11
//   +2005 PF5.51 worst -147bp (short_thr 0.64 / trail 240 / real 28bp taker cost).
//   Maker-only re-BT (CORE_MAKER_ONLY_FINDINGS_2026-07-15) confirmed taker-RT 28bp basis.
//
// PURPOSE: reproduce, one-bar-at-a-time on the LIVE aggTrade side feed, the exact CORE
//   trigger state machine validated in Crypto/backtest/core_trigger_p2_bt.cpp. Variable-
//   for-variable with run_core so a PARITY TEST (same 15m bars in → identical ETH+XRP
//   trades) can gate the deploy. SHADOW only — emits ClipRecord on each closed trade;
//   opens/moves/closes NOTHING real (whole crypto book is shadow $0).
//
// MODEL FIDELITY: full per-coin bar vectors are retained (not ring buffers) so trend_up /
//   atr_bps / comp-window lookbacks are computed with the identical incremental formulas
//   as the backtest — guaranteed index-exact. BTC regime is keyed by 15m SLOT (time-
//   aligned) rather than bar-count so a live feed gap can't silently misalign the gate;
//   on a contiguous grid (the parity feed, and normal live) slot==index so parity holds.
//   Memory: ~4 doubles/bar/coin; a shadow engine is restarted well before this matters.
//
// SEED SEMANTICS: seed bars fill the SMA16/64 + ATR14 + comp-window buffers with
//   enabled-suppressed state (no historical entries), matching the warm-seed mandate.
//   Live REST klines drop taker_buy_base, so seed bars carry tbb_frac=0.5 (neutral) — the
//   flow gate (short_thr/med_thr) is only evaluated on LIVE bars whose tbb is real.

#pragma once
#include "SymbolIndex.hpp"
#include "MimicLadderCompanion.hpp"   // ClipRecord / ClipCallback contract (reused)
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace chimera {

class CoreTriggerEngine {
public:
    using ClipRecord   = MimicLadderCompanion::ClipRecord;
    using ClipCallback = MimicLadderCompanion::ClipCallback;

    // Per-coin cell config — defaults = the validated ETH/XRP passing cell.
    struct Cell {
        std::string symbol;                 // e.g. "ethusdt"
        std::string tag;                    // e.g. "CORE-ETH"
        SymbolId    id = SYM_BTC;           // set from symbol in ctor
        double comp_w = 80.0, short_thr = 0.64, med_thr = 0.56, move_mult = 3.0, pb_maxfrac = 0.50;
        double safe_ref = 30.0, trailmin = 240.0, trailatr = 0.5, stopbuf = 5.0;
        int    comp_bars = 6, short_w = 3, med_w = 8, w1 = 16, w4 = 64;
        int    maxwait = 24, cooldown = 8, btc_gate = 1, room_h = 8;
        double qusd = 100000.0, cost_rt_bp = 28.0;   // taker RT (fee+slip+reserve), validated
        bool   enabled = true;
    };

    struct Config {
        std::string btc_symbol = "btcusdt";   // regime driver
        int         tf_secs     = 900;         // 15m bars
        std::vector<Cell> cells;
    };

    explicit CoreTriggerEngine(Config cfg) : cfg_(std::move(cfg)) {
        btc_id_ = symbol_to_id(cfg_.btc_symbol);
        for (auto& c : cfg_.cells) {
            c.id = symbol_to_id(c.symbol);
            states_.emplace_back();
            id_to_cell_[(int)c.id] = (int)states_.size() - 1;
        }
    }

    void set_on_clip(ClipCallback cb) { on_clip_ = std::move(cb); }
    const Config& config() const { return cfg_; }
    size_t cell_count() const { return cfg_.cells.size(); }
    int slot_ms() const { return cfg_.tf_secs * 1000; }

    // ---- live trade ingress (from main.cpp aggTrade callback; guard trade_qty>0 upstream)
    void on_trade(SymbolId id, double px, double qty, bool is_buyer_maker, int64_t ts_ms) {
        if (px <= 0.0 || qty <= 0.0 || ts_ms <= 0) return;
        if ((int)id == (int)btc_id_) { agg_btc_.push(px, qty, is_buyer_maker, ts_ms, slot_ms()); flush_btc_(/*seed*/false); return; }
        auto it = id_to_cell_.find((int)id);
        if (it == id_to_cell_.end()) return;
        int ci = it->second;
        states_[ci].agg.push(px, qty, is_buyer_maker, ts_ms, slot_ms());
        flush_coin_(ci, /*seed*/false);
    }

    // ---- warm-seed one closed historical bar (enabled-suppressed; tbb_frac neutral 0.5 ok)
    void seed_bar(SymbolId id, int64_t slot, double o, double h, double l, double c, double tbb_frac) {
        if ((int)id == (int)btc_id_) { ingest_btc_(slot, o, h, l, c); return; }
        auto it = id_to_cell_.find((int)id);
        if (it == id_to_cell_.end()) return;
        ingest_coin_(it->second, slot, o, h, l, c, tbb_frac, /*seed*/true);
    }

    // ---- PARITY / offline replay: ingest a fully-formed bar and run the state machine live.
    void ingest_bar(SymbolId id, int64_t slot, double o, double h, double l, double c, double tbb_frac) {
        if ((int)id == (int)btc_id_) { ingest_btc_(slot, o, h, l, c); return; }
        auto it = id_to_cell_.find((int)id);
        if (it == id_to_cell_.end()) return;
        ingest_coin_(it->second, slot, o, h, l, c, tbb_frac, /*seed*/false);
    }

private:
    // ------- intrabar aggregator: ticks -> one 15m OHLC + taker-buy-base fraction -------
    struct BarAgg {
        int64_t slot = -1; double o=0,h=0,l=0,c=0,vol=0,tbb=0; bool open=false;
        int64_t pending_slot=-1; double po=0,ph=0,pl=0,pc=0,pvol=0,ptbb=0; bool have_pending=false;
        void push(double px, double qty, bool is_buyer_maker, int64_t ts_ms, int slot_ms_) {
            int64_t s = ts_ms - (ts_ms % slot_ms_);
            if (!open) { slot=s; o=h=l=c=px; vol=qty; tbb=(is_buyer_maker?0.0:qty); open=true; return; }
            if (s != slot) {   // roll: freeze the completed bar for the consumer to drain
                pending_slot=slot; po=o; ph=h; pl=l; pc=c; pvol=vol; ptbb=tbb; have_pending=true;
                slot=s; o=h=l=c=px; vol=qty; tbb=(is_buyer_maker?0.0:qty); return;
            }
            if (px>h) h=px; if (px<l) l=px; c=px; vol+=qty; if(!is_buyer_maker) tbb+=qty;
        }
    };

    struct CellState {
        BarAgg agg;
        // series (index-exact with the backtest)
        std::vector<int64_t> ts; std::vector<double> h,l,c,tbbf,tr,atr;
        std::vector<char> up1,up4;
        double sma1=0.0, sma4=0.0, atrsum=0.0;   // rolling sums
        // run_core state (verbatim variable names)
        int st=0; // 0 SCAN 1 COMP 2 BROKE 3 INPOS
        double rhi=0,rlo=0,anchor_px=0,vwap_num=0,vwap_den=0;
        double impulse=0,peak=0,pb_low=0; int brk_i=0; bool pulled=false;
        double entry=0,stop=0,ppeak=0; int cool_until=-1; int entry_i=0;
        int clip_num=0;
    };

    // ---- BTC regime: closes -> up1/up4 by slot (time-aligned gate) ----
    BarAgg agg_btc_;
    std::vector<double> btc_c_; double btc_sma1_=0, btc_sma4_=0;
    std::unordered_map<int64_t,char> btc_up1_slot_, btc_up4_slot_;
    int64_t btc_last_slot_=-1; char btc_last_up1_=0, btc_last_up4_=0;

    void flush_btc_(bool seed) {
        while (agg_btc_.have_pending) {
            agg_btc_.have_pending=false;
            ingest_btc_(agg_btc_.pending_slot, agg_btc_.po, agg_btc_.ph, agg_btc_.pl, agg_btc_.pc);
            (void)seed;
        }
    }
    void ingest_btc_(int64_t slot, double /*o*/, double /*hh*/, double /*ll*/, double cc) {
        int i = (int)btc_c_.size();
        btc_c_.push_back(cc);
        btc_sma1_ += cc; if (i>=w1_()) btc_sma1_ -= btc_c_[i-w1_()];
        btc_sma4_ += cc; if (i>=w4_()) btc_sma4_ -= btc_c_[i-w4_()];
        char u1 = (i>=w1_() && cc > btc_sma1_/w1_()) ? 1 : 0;
        char u4 = (i>=w4_() && cc > btc_sma4_/w4_()) ? 1 : 0;
        btc_up1_slot_[slot]=u1; btc_up4_slot_[slot]=u4;
        btc_last_slot_=slot; btc_last_up1_=u1; btc_last_up4_=u4;
    }
    // w1/w4 for BTC taken from the first cell (all cells share 16/64 in the validated cfg)
    int w1_() const { return cfg_.cells.empty()?16:cfg_.cells[0].w1; }
    int w4_() const { return cfg_.cells.empty()?64:cfg_.cells[0].w4; }
    bool btc_up_at(int64_t slot, char& u1, char& u4) const {
        auto a=btc_up1_slot_.find(slot), b=btc_up4_slot_.find(slot);
        if (a!=btc_up1_slot_.end() && b!=btc_up4_slot_.end()) { u1=a->second; u4=b->second; return true; }
        if (btc_last_slot_>=0 && slot>=btc_last_slot_) { u1=btc_last_up1_; u4=btc_last_up4_; return true; } // latest≤slot
        return false;
    }

    // ---- coin: drain aggregator, ingest completed bars ----
    void flush_coin_(int ci, bool seed) {
        BarAgg& a = states_[ci].agg;
        while (a.have_pending) {
            a.have_pending=false;
            ingest_coin_(ci, a.pending_slot, a.po, a.ph, a.pl, a.pc, a.pvol>0? a.ptbb/a.pvol : 0.5, seed);
        }
    }

    void ingest_coin_(int ci, int64_t slot, double /*o*/, double hh, double ll, double cc,
                      double tbb_frac, bool seed) {
        CellState& s = states_[ci];
        const Cell& p = cfg_.cells[ci];
        int i = (int)s.c.size();
        // append bar + incremental trend_up / atr_bps (identical formulas to the backtest)
        s.ts.push_back(slot); s.h.push_back(hh); s.l.push_back(ll); s.c.push_back(cc); s.tbbf.push_back(tbb_frac);
        double pc = (i>0)? s.c[i-1] : cc;
        double t  = std::max(hh-ll, std::max(std::fabs(hh-pc), std::fabs(ll-pc)));
        s.tr.push_back(t);
        s.sma1 += cc; if (i>=p.w1) s.sma1 -= s.c[i-p.w1];
        s.sma4 += cc; if (i>=p.w4) s.sma4 -= s.c[i-p.w4];
        s.up1.push_back((i>=p.w1 && cc > s.sma1/p.w1)?1:0);
        s.up4.push_back((i>=p.w4 && cc > s.sma4/p.w4)?1:0);
        s.atrsum += t; if (i>=14) s.atrsum -= s.tr[i-14];
        s.atr.push_back((i>=14 && cc>0)? (s.atrsum/14.0)/cc*1e4 : 0.0);

        if (seed) { s.st=0; return; }           // seed: warm buffers only, no state machine
        int warm = std::max(p.w4, p.comp_bars) + 2;
        if (i < warm) return;
        step_(ci, i);
    }

    // ---- the run_core for-loop BODY, verbatim, for the current bar index i ----
    void step_(int ci, int i) {
        CellState& s = states_[ci];
        const Cell& p = cfg_.cells[ci];
        auto& h=s.h; auto& l=s.l; auto& c=s.c; auto& tbb=s.tbbf; auto& up1=s.up1; auto& up4=s.up4; auto& atr=s.atr;

        if (s.st==1||s.st==2){ double tp=(h[i]+l[i]+c[i])/3.0; s.vwap_num+=tp; s.vwap_den+=1; }
        double vwap = s.vwap_den>0? s.vwap_num/s.vwap_den : c[i];

        if (s.st==0){ // SCAN
            if (i<=s.cool_until) return;
            int a=i-p.comp_bars+1; double mh=-1e18,ml=1e18;
            for(int j=a;j<=i;++j){ mh=std::max(mh,h[j]); ml=std::min(ml,l[j]); }
            double mid=(mh+ml)/2.0; double w=mid>0?(mh-ml)/mid*1e4:1e9;
            if(w<=p.comp_w){ s.rhi=mh; s.rlo=ml; s.anchor_px=mid; s.st=1; s.vwap_num=0; s.vwap_den=0; }
        }
        else if (s.st==1){ // COMP
            double sh=0; { int a=std::max(0,i-p.short_w+1),n=0; for(int j=a;j<=i;++j){sh+=tbb[j];n++;} sh/=(n>0?n:1);}
            double md=0; { int a=std::max(0,i-p.med_w+1),n=0; for(int j=a;j<=i;++j){md+=tbb[j];n++;} md/=(n>0?n:1);}
            if(c[i]>s.rhi && sh>=p.short_thr && md>=p.med_thr){
                s.impulse=(c[i]-s.rlo)/s.rlo*1e4;
                s.st=2; s.brk_i=i; s.peak=h[i]; s.pb_low=l[i]; s.pulled=false;
            } else if(l[i] < s.rlo){ s.st=0; }
        }
        else if (s.st==2){ // BROKE
            if(i-s.brk_i>p.maxwait){ s.st=0; return; }
            if(!s.pulled) s.peak=std::max(s.peak,h[i]);
            double hold = std::max(s.rhi,vwap);
            if(l[i] < hold){ s.st=0; return; }
            double giveback_bps = (s.peak-c[i])/s.peak*1e4;
            if(!s.pulled){ if(giveback_bps >= std::max(15.0,0.25*s.impulse)){ s.pulled=true; s.pb_low=l[i]; } }
            else { s.pb_low=std::min(s.pb_low,l[i]);
                double pbdepth=(s.peak-s.pb_low)/s.peak*1e4;
                if(pbdepth > p.pb_maxfrac*s.impulse + p.safe_ref){ s.st=0; return; }
                if(c[i] > s.peak){
                    bool g_up = up1[i] && up4[i];
                    bool g_vwap = c[i] > vwap;
                    bool g_btc = true;
                    if(p.btc_gate){ char b1=1,b4=1; if(btc_up_at(s.ts[i],b1,b4)) g_btc=(b1&&b4); else g_btc=true; }
                    bool g_hl = s.pb_low > s.rhi;
                    bool g_room = atr[i]*p.room_h >= p.move_mult*p.safe_ref;
                    if(g_up&&g_vwap&&g_btc&&g_hl&&g_room){
                        s.st=3; s.entry=c[i]; s.ppeak=c[i]; s.entry_i=i;
                        s.stop=s.pb_low*(1.0 - p.stopbuf/1e4);
                    } else { s.st=0; }
                }
            }
        }
        else if (s.st==3){ // INPOS
            s.ppeak=std::max(s.ppeak,h[i]);
            double vw = vwap;
            double trail=std::max(p.trailmin, p.trailatr*atr[i]);
            double exit_px=0; bool ex=false;
            if(l[i] <= s.stop){ exit_px=s.stop; ex=true; }
            else { double gb=(s.ppeak-c[i])/s.ppeak*1e4;
                   if(gb>=trail){ exit_px=c[i]; ex=true; }
                   else if(c[i] < vw){ exit_px=c[i]; ex=true; } }
            if(ex){
                emit_clip_(ci, i, exit_px);
                s.st=0; s.cool_until=i+p.cooldown; s.vwap_num=0; s.vwap_den=0;
            }
        }
    }

    void emit_clip_(int ci, int i, double exit_px) {
        CellState& s = states_[ci];
        const Cell& p = cfg_.cells[ci];
        double gross = (exit_px - s.entry)/s.entry*1e4;
        double net   = gross - p.cost_rt_bp;
        ClipRecord r;
        r.tag = p.tag; r.symbol = p.symbol; r.reason = "CORE_EXIT";
        r.entry_ts_ms = s.ts[s.entry_i]; r.exit_ts_ms = s.ts[i];
        r.entry_px = s.entry; r.exit_px = exit_px;
        r.gross_bp = gross; r.net_bp = net;
        r.gross_bp_real = gross; r.net_bp_real = net;   // shadow: model==real (no fill divergence)
        r.mfe_pct = (s.ppeak/s.entry - 1.0)*100.0;
        r.size_mult = 1.0;
        r.bars_held = i - s.entry_i; r.clip_num = ++s.clip_num;
        r.shadow = true;
        if (on_clip_) on_clip_(r);
    }

    Config cfg_;
    SymbolId btc_id_ = SYM_BTC;
    std::vector<CellState> states_;
    std::unordered_map<int,int> id_to_cell_;
    ClipCallback on_clip_;
};

} // namespace chimera

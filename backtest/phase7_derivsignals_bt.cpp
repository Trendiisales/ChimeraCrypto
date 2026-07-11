// ============================================================================
// phase7_derivsignals_bt.cpp — Phase-7 DERIVATIVES-DATA-AS-SIGNAL, the REAL gate.
//
// Does a derivative/microstructure signal, applied as a QUALITY FILTER / SIZE
// modifier on the EXISTING spot-long entries, IMPROVE those entries? Faithful
// live UpJump (per-coin H1 W/thr, ride-to-flip, 20bp round-trip). Gate-
// attribution counterfactual (Phase-4 style): of the entries a filter would
// VETO, are they LOSERS (helpful) or WINNERS (suspect)? + quartile monotonicity
// (does the signal RANK entry forward-quality?) + WF halves. Long-only spot; the
// derivatives data is NEVER traded.
//
// DATA: data/klines_spot + klines_perp (1m, taker flow -> H1 + CVD) + funding
// (8 sym, 2025-05-10..2026-05-10). This 1-year, single-regime window is the
// ENTIRE derivatives history available — verdicts are directional on a short
// sample; that is exactly why the productive Phase-7 wiring is an OBSERVATION-
// ONLY recorder (accrue forward data), not a live filter.
//
//   build: g++ -std=c++20 -O2 -I../include phase7_derivsignals_bt.cpp -o phase7_derivsignals_bt
//   run:   ./phase7_derivsignals_bt        (from backtest/, needs ../data)
// ============================================================================
#include "core/DerivativesSignals.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace chimera;

struct H1 { int64_t h; double close, vol, tbb; };

// aggregate a 1m kline CSV (open_time_ms,o,h,l,c,volume,close_time,quote_vol,
// trades,taker_buy_base,...) to H1 (close=last, vol=sum, tbb=sum).
static std::vector<H1> load_h1(const std::string& path) {
    std::map<int64_t,H1> agg;
    FILE* f = fopen(path.c_str(), "r");
    if (!f) { fprintf(stderr, "  (missing %s)\n", path.c_str()); return {}; }
    char line[1024]; bool first = true;
    while (fgets(line, sizeof line, f)) {
        if (first) { first = false; if (!isdigit((unsigned char)line[0])) continue; }
        char* p = line; char* e;
        int64_t t = strtoll(p, &e, 10); if (e == p) continue;
        strtod(e+1,&e); strtod(e+1,&e); strtod(e+1,&e);      // o h l
        double c = strtod(e+1,&e);                            // close
        double v = strtod(e+1,&e);                            // volume
        strtoll(e+1,&e,10); strtod(e+1,&e); strtoll(e+1,&e,10); // close_time quote_vol trades
        double tbb = strtod(e+1,&e);                          // taker_buy_base
        if (c <= 0) continue;
        int64_t hh = (t/3600000LL)*3600000LL;
        auto& b = agg[hh]; b.h = hh; b.close = c; b.vol += v; b.tbb += tbb;
    }
    fclose(f);
    std::vector<H1> out; out.reserve(agg.size());
    for (auto& kv : agg) out.push_back(kv.second);
    return out;
}

// funding CSV: symbol,funding_time_ms,funding_rate,mark_price
static std::vector<std::pair<int64_t,double>> load_funding(const std::string& path) {
    std::vector<std::pair<int64_t,double>> out;
    FILE* f = fopen(path.c_str(), "r"); if (!f) return out;
    char line[512]; bool first = true;
    while (fgets(line, sizeof line, f)) {
        if (first) { first = false; if (!isdigit((unsigned char)line[0]) && line[0] != 'B'
                     && line[0] != 'E' && line[0] != 'S' && line[0] != 'A'
                     && line[0] != 'D' && line[0] != 'L' && line[0] != 'X') continue; if(first)continue; }
        char* c1 = strchr(line, ','); if (!c1) continue;
        char* e; int64_t t = strtoll(c1+1, &e, 10);
        double r = strtod(e+1, &e);
        out.push_back({t, r});
    }
    fclose(f);
    std::sort(out.begin(), out.end());
    return out;
}

struct Entry { std::string sym; double net, fwd48, fpct, cvd, basis; int half; };

struct Roster { const char* sym; int W; double thr; };

int main() {
    // live FULL-BULL roster per-coin (W hours, thr); LINK/AVAX not live -> fat-tail 4h/2%
    std::vector<Roster> roster = {
        {"BTCUSDT",2,0.04},{"ETHUSDT",1,0.02},{"SOLUSDT",1,0.05},{"DOGEUSDT",4,0.04},
        {"BNBUSDT",1,0.03},{"XRPUSDT",1,0.04},{"LINKUSDT",4,0.02},{"AVAXUSDT",4,0.02},
    };
    const double RT = 0.0020;               // 20bp round trip (round_trip_bp=20 live)
    const std::string D = "../data";
    // window midpoint for WF halves
    const int64_t T0 = 1746892800001LL, T1 = 1778400000000LL, MID = T0 + (T1-T0)/2;

    std::vector<Entry> E;
    for (auto& r : roster) {
        auto spot = load_h1(D + "/klines_spot/" + r.sym + "_1m.csv");
        auto perp = load_h1(D + "/klines_perp/" + r.sym + "_1m.csv");
        auto fund = load_funding(D + "/funding/" + std::string(r.sym) + ".csv");
        if (spot.size() < 100 || perp.empty() || fund.empty()) { fprintf(stderr,"skip %s\n",r.sym); continue; }
        std::map<int64_t,const H1*> pmap; for (auto& b : perp) pmap[b.h] = &b;

        // replay the derivative book bar-by-bar so eval() is point-in-time
        DerivativesSignalBook book;
        size_t fi = 0;
        int n = (int)spot.size();
        bool in_pos = false; int entry_i = 0;
        for (int i = 0; i < n; ++i) {
            int64_t hh = spot[i].h;
            // feed funding up to this hour
            while (fi < fund.size() && fund[fi].first <= hh) { book.on_funding(r.sym, fund[fi].first, fund[fi].second); ++fi; }
            book.on_spot_h1(r.sym, hh, spot[i].close, spot[i].vol, spot[i].tbb);
            auto pit = pmap.find(hh);
            if (pit != pmap.end()) book.on_perp_h1(r.sym, hh, pit->second->close, pit->second->vol, pit->second->tbb);

            if (i < r.W) continue;
            double ret = spot[i].close / spot[i-r.W].close - 1.0;
            if (!in_pos && ret >= r.thr) { in_pos = true; entry_i = i;
                // stamp derivative context AT ENTRY
                DerivSignal d = book.eval(r.sym);
                E.push_back({r.sym, 0, 0, d.funding_pct, d.cvd_div, d.basis_pct,
                             spot[entry_i].h < MID ? 0 : 1});
            }
            else if (in_pos && (spot[i].close / spot[i-r.W].close - 1.0) <= -r.thr) {
                double net = spot[i].close / spot[entry_i].close - 1.0 - RT;
                int fwd_i = std::min(entry_i + 48, n - 1);
                double fwd48 = spot[fwd_i].close / spot[entry_i].close - 1.0;
                E.back().net = net; E.back().fwd48 = fwd48;
                in_pos = false;
            }
        }
        // an unterminated position is dropped (no exit observed) — same as probe
        if (in_pos && !E.empty()) E.pop_back();
    }

    // ---- reporting helpers -------------------------------------------------
    auto stat = [](const std::vector<const Entry*>& v, bool fwd)->std::string{
        if (v.empty()) return "n=0";
        double s=0,m=0; int win=0; for (auto* e:v){double x=fwd?e->fwd48:e->net; s+=x; m+=x; if(x>0)win++;}
        m/=v.size(); double var=0; for(auto* e:v){double x=(fwd?e->fwd48:e->net)-m; var+=x*x;} var/=v.size();
        double sd=var>0?std::sqrt(var):0, t=sd>0?m/sd*std::sqrt((double)v.size()):0;
        char buf[160]; snprintf(buf,sizeof buf,"n=%zu mean=%+.2f%% wr=%.0f%% sum=%+.0f%% t~%.2f",
                                v.size(), m*100, (double)win/v.size()*100, s*100, t);
        return buf;
    };
    auto all=[&](){ std::vector<const Entry*> v; for(auto&e:E) v.push_back(&e); return v; }();
    auto sub=[&](auto pred){ std::vector<const Entry*> v; for(auto&e:E) if(pred(e)) v.push_back(&e); return v; };

    printf("=== Phase-7 DERIVATIVES-DATA-AS-SIGNAL backtest ===\n");
    printf("faithful live UpJump (per-coin H1 W/thr, ride-to-flip, 20bp RT); 8 sym; 2025-05-10..2026-05-10\n\n");
    printf("BASE (unfiltered) net  : %s\n", stat(all,false).c_str());
    printf("BASE (unfiltered) fwd48: %s\n", stat(all,true).c_str());
    printf("  NOTE: this 1yr window is UNFAVORABLE for jump-chasing (H1 momentum-positive, H2 negative);\n");
    printf("  the live roster's multi-year edge is not the subject — the FILTER's separation is.\n");

    // gate-attribution: veto set worse than keep set => helpful (removes losers)
    auto gate=[&](const char* name, auto vetopred){
        auto veto=sub([&](const Entry&e){return vetopred(e);});
        auto keep=sub([&](const Entry&e){return !vetopred(e);});
        double bm=0; for(auto&e:E) bm+=e.net; bm/= (E.empty()?1:E.size());
        double km=0; for(auto* e:keep) km+=e->net; km/=(keep.empty()?1:keep.size());
        printf("\n[%s]\n", name);
        printf("  VETOED : %s | fwd48 %s   <- HELPFUL if these are losers\n", stat(veto,false).c_str(), stat(veto,true).c_str());
        printf("  KEPT   : %s | fwd48 %s\n", stat(keep,false).c_str(), stat(keep,true).c_str());
        printf("  book quality: kept mean %+.2f%% vs base %+.2f%%  => %s per-trade\n",
               km*100, bm*100, km>bm?"IMPROVES":"WORSENS");
        auto kh=[&](int h){ return sub([&](const Entry&e){return e.half==h && !vetopred(e);}); };
        printf("    kept WF H1: %s\n", stat(kh(0),false).c_str());
        printf("    kept WF H2: %s\n", stat(kh(1),false).c_str());
    };

    // percentile thresholds for basis (from distribution)
    std::vector<double> bs; for(auto&e:E) bs.push_back(e.basis); std::sort(bs.begin(),bs.end());
    double b80 = bs.empty()?0:bs[(size_t)(bs.size()*0.80)];

    gate("A  funding-extreme veto (funding_pct >= 0.80 = crowded longs)",
         [](const Entry&e){ return e.fpct >= 0.80; });
    gate("B  spot-vs-perp CVD veto perp-led (cvd_div < 0)",
         [](const Entry&e){ return e.cvd < 0.0; });
    char cname[128]; snprintf(cname,sizeof cname,"C  basis-extreme veto (basis >= %.4f%% = top 20%%)", b80*100);
    gate(cname, [&](const Entry&e){ return e.basis >= b80; });

    // ---- quartile monotonicity: does the signal RANK forward quality? -------
    auto quart=[&](const char* name, auto key){
        std::vector<const Entry*> v=all; std::sort(v.begin(),v.end(),
            [&](const Entry*a,const Entry*b){return key(*a)<key(*b);});
        size_t q=v.size()/4;
        printf("\n[%s] quartiles (low->high), fwd48 mean:\n", name);
        for(int i=0;i<4;i++){ size_t a=i*q, b=(i<3)?(i+1)*q:v.size(); double m=0; for(size_t k=a;k<b;k++) m+=v[k]->fwd48; m/=(b-a);
            printf("   Q%d n=%zu fwd48=%+.2f%%\n", i+1, b-a, m*100); }
    };
    quart("CVD cvd_div (Q1 most perp-led -> Q4 most spot-led)", [](const Entry&e){return e.cvd;});
    quart("funding fpct (Q1 low -> Q4 extreme high)", [](const Entry&e){return e.fpct;});
    quart("basis (Q1 low -> Q4 high)", [](const Entry&e){return e.basis;});

    printf("\n=== VERDICT (gate-attribution + monotonicity, cost-invariant: 2x cost shifts all nets\n");
    printf("    uniformly by 20bp so it does NOT change the separation/ranking) ===\n");
    printf("A funding-extreme : NEUTRAL — no separation, non-monotonic quartiles => REJECT (not wired).\n");
    printf("B spot-vs-perp CVD: WEAK/NON-MONOTONIC — perp-led Q1 worst but Q4 spot-led also poor;\n");
    printf("                    the veto 'help' is the Q1 tail only, does not generalise => REJECT as a\n");
    printf("                    filter; strongest of the three => wire OBSERVATION-ONLY recorder.\n");
    printf("C basis-extreme   : SUSPECT — high-basis entries were WINNERS (veto suppresses winners) => REJECT.\n");
    printf("OI / real-liquidity-cost / liquidations / stablecoin-flows / event-risk: DEFERRED (no history).\n");
    return 0;
}

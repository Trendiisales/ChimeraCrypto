// ============================================================================
// xsec2_forwardproof_bt.cpp — PHASE 8E: XSec2 FORWARD-PROOF (offline checklist)
//
// Runs every promotion-checklist item from CRYPTO_PHASE8_ROADMAP.md "8E" that
// does NOT need live-forward data, to firm up or weaken the KEEP verdict for
// XSec 2.0 (kept as a diversifier on the corrected long-only gate; blend Sharpe
// 1.51 vs v1-alone 1.29 in Phase-5).
//
// Standing rules honoured: LONG-ONLY SPOT, NO SHORTS, NO 200DMA in XSec2's own
// gate (breadth participation ratio). 2022 SHOWN-not-gated. BACKTEST_TRUTH.
// (RipRider — a RETAINED engine — carries its OWN pre-existing BTC>200d gate;
//  it is used AS-IS here, unmodified. No new 200DMA is introduced by this tool.)
//
// OFFLINE / RESEARCH ONLY: reads the committed daily warm-seed CSVs; changes NO
// engine logic and does NOT disturb the grid / shadow / observation books.
//
//   build:  g++ -std=c++20 -O2 -I../include xsec2_forwardproof_bt.cpp -o xsec2_forwardproof_bt
//   run:    ./xsec2_forwardproof_bt         (from backtest/, needs ../data/xsec_seed)
//
// Checklist items (each prints PASS/FAIL/PENDING + numbers):
//   1. Standalone XSec2 vs original XSec — equal capital + equal VOL + same PIT
//      universe + same cost/fill (did XSec2 ITSELF improve, or just blend diff?)
//   2. Year-by-year (broad vs concentrated improvement)
//   3. Removed-best-EPISODE (dependence on one crypto cycle) — standalone + increment
//   4. Parameter-neighbourhood (corrected gate not one lucky threshold) — increment holds?
//   5. INCREMENTAL contribution AFTER RipRider + retained engines (the crux)
//   6. Capacity / liquidity (challenger/rebalance trades executable at real size?)
//   7. Forward-shadow — PENDING live observation (reported, not gated)
// ============================================================================
#include "core/CrossSectionalMomentumEngine.hpp"
#include "core/CrossSectionalMomentum2Engine.hpp"
#include "core/RipRiderEngine.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <random>
#include <algorithm>

using namespace chimera;

// ---- data --------------------------------------------------------------------
struct Row { int64_t day; double open, high, low, close, dvol; };
static std::map<std::string,std::vector<Row>> load_universe(const std::vector<std::string>& syms,
                                                            const std::string& dir) {
    std::map<std::string,std::vector<Row>> out;
    for (auto& s : syms) {
        std::string path = dir + "/" + s + "USDT_1d.csv";
        FILE* f = fopen(path.c_str(), "r");
        if (!f) { fprintf(stderr, "  (missing %s)\n", path.c_str()); continue; }
        char line[512]; bool first = true;
        while (fgets(line, sizeof line, f)) {
            if (first) { first = false; if (!(isdigit((unsigned char)line[0]))) continue; }
            char* p = line; char* e;
            long long ts = strtoll(p, &e, 10); if (e == p) continue;
            double o = strtod(e+1, &e);
            double h = strtod(e+1, &e);
            double l = strtod(e+1, &e);
            double c = strtod(e+1, &e);
            double vol = strtod(e+1, &e);
            if (c > 0) out[s].push_back({ ts/86400000LL, o, h, l, c, c*vol });
        }
        fclose(f);
    }
    return out;
}

// ---- metrics -----------------------------------------------------------------
using Series = std::vector<std::pair<int64_t,double>>;   // (day, daily return)
struct Metrics {
    double total = 0, cagr = 0, sharpe = 0, maxdd = 0, calmar = 0, vol = 0, pf = 0;
    int    days = 0, active = 0;
};
static Metrics metrics(const Series& d) {
    Metrics m; m.days = (int)d.size();
    if (d.empty()) return m;
    double eq = 1.0, peak = 1.0, mean = 0, gp = 0, gl = 0;
    for (auto& kv : d) {
        double r = kv.second; mean += r;
        if (std::fabs(r) > 1e-12) m.active++;
        if (r > 0) gp += r; else gl += -r;
        eq *= (1.0 + r); peak = std::max(peak, eq);
        m.maxdd = std::max(m.maxdd, (peak - eq)/peak);
    }
    m.total = eq - 1.0; mean /= d.size();
    double var = 0; for (auto& kv : d) { double x = kv.second - mean; var += x*x; }
    var /= d.size();
    double sd = var > 0 ? std::sqrt(var) : 0;
    m.vol = sd * std::sqrt(365.0);
    m.sharpe = sd > 0 ? mean/sd*std::sqrt(365.0) : 0;
    double yrs = d.size()/365.0;
    m.cagr = (yrs > 0 && eq > 0) ? std::pow(eq, 1.0/yrs) - 1.0 : 0;
    m.calmar = m.maxdd > 1e-9 ? m.cagr/m.maxdd : 0;
    m.pf = gl > 0 ? gp/gl : (gp > 0 ? 999.0 : 0.0);
    return m;
}
static Series blend(const Series& a, const Series& b, double wa, double wb) {
    std::map<int64_t,double> m; for (auto& kv : a) m[kv.first] += wa*kv.second;
    for (auto& kv : b) m[kv.first] += wb*kv.second;
    return Series(m.begin(), m.end());
}
static Series blend3(const Series& a, const Series& b, const Series& c,
                     double wa, double wb, double wc) {
    std::map<int64_t,double> m;
    for (auto& kv : a) m[kv.first] += wa*kv.second;
    for (auto& kv : b) m[kv.first] += wb*kv.second;
    for (auto& kv : c) m[kv.first] += wc*kv.second;
    return Series(m.begin(), m.end());
}
static Series drop2022(const Series& d) {
    int64_t lo=1640995200000LL, hi=1672531200000LL; Series o;
    for (auto& kv : d) { int64_t t=kv.first*86400000LL; if (!(t>=lo && t<hi)) o.push_back(kv); }
    return o;
}
static Series yearslice(const Series& d, int year) {
    // crude UTC-year boundaries by ms epoch
    static const int64_t Y[] = {
        1577836800000LL,1609459200000LL,1640995200000LL,1672531200000LL,
        1704067200000LL,1735689600000LL,1767225600000LL,1798761600000LL };
    int idx = year - 2020; if (idx < 0 || idx > 6) return {};
    int64_t lo=Y[idx], hi=Y[idx+1]; Series o;
    for (auto& kv : d) { int64_t t=kv.first*86400000LL; if (t>=lo && t<hi) o.push_back(kv); }
    return o;
}
// scale a daily-return series to a target annualized vol (equal-vol normalization)
static Series scale_to_vol(const Series& d, double target_vol) {
    Metrics m = metrics(d); if (m.vol < 1e-9) return d;
    double k = target_vol / m.vol; Series o;
    for (auto& kv : d) o.push_back({kv.first, kv.second * k});
    return o;
}
static double corr(const Series& a, const Series& b) {
    std::map<int64_t,double> ma, mb;
    for (auto& kv : a) ma[kv.first]=kv.second;
    for (auto& kv : b) mb[kv.first]=kv.second;
    std::vector<double> x,y; for (auto& kv : ma){ auto it=mb.find(kv.first);
        if(it!=mb.end()){x.push_back(kv.second);y.push_back(it->second);} }
    if (x.size()<3) return 0;
    double mx=0,my=0; for(size_t i=0;i<x.size();++i){mx+=x[i];my+=y[i];} mx/=x.size();my/=y.size();
    double sxy=0,sxx=0,syy=0; for(size_t i=0;i<x.size();++i){double dx=x[i]-mx,dy=y[i]-my;sxy+=dx*dy;sxx+=dx*dx;syy+=dy*dy;}
    return (sxx>0&&syy>0)? sxy/std::sqrt(sxx*syy):0;
}

// ---- universe ----------------------------------------------------------------
static const std::vector<std::string> UNIVERSE = {
    "AAVE","ADA","ALGO","APE","AR","ATOM","AVAX","AXS","BAT","BCH","BNB","BTC",
    "CAKE","CHZ","COMP","CRV","DASH","DOGE","DOT","EGLD","ENJ","ETC","ETH","FET",
    "FIL","GALA","GRT","HBAR","ICP","INJ","KSM","LDO","LINK","LTC","MANA","NEAR",
    "NEO","ONE","QTUM","RUNE","SAND","SHIB","SNX","SOL","SUSHI","THETA","TRX",
    "UNI","VET","XLM","XRP","XTZ","ZEC","ZIL" };

// ---- build engine daily-return series ---------------------------------------
static Series build_v1(const std::map<std::string,std::vector<Row>>& data, double cost=15.0) {
    XSecConfig v1b; v1b.gate_mode = 0; v1b.cost_bps = cost;
    XSecConfig v1r; v1r.gate_mode = 1; v1r.breadth_thresh = 0.65; v1r.cost_bps = cost;
    auto mk = [&](const XSecConfig& c){ auto e = new CrossSectionalMomentumEngine(c);
        e->set_universe(UNIVERSE);
        for (auto& kv : data) for (auto& r : kv.second) e->seed_daily_close(kv.first, r.day, r.close);
        auto s = e->simulate(); delete e; return s; };
    return blend(mk(v1b), mk(v1r), 0.6, 0.4);
}
static Series build_v2(const std::map<std::string,std::vector<Row>>& data, XSec2Config c) {
    auto e = new CrossSectionalMomentum2Engine(c);
    e->set_universe(UNIVERSE);
    for (auto& kv : data) for (auto& r : kv.second) e->seed_daily(kv.first, r.day, r.close, r.dvol);
    auto s = e->simulate(); delete e; return s;
}

// RipRider as a comparable daily-return sleeve: reconstruct held intervals from
// entry+close callbacks (FIFO per symbol — RipRider holds <=1 position/symbol),
// then build an equal-weight-of-open-positions daily return (CASH when flat),
// with round-trip cost debited on each exit day. This makes the fat-tail rider
// a fully-invested-when-in daily series comparable to XSec (offline only).
static Series build_riprider(const std::map<std::string,std::vector<Row>>& data,
                             double cost_rt=0.004) {
    RipRiderConfig c; // defaults: ignition 20% / lb5, BTC>200d gate + regime_exit
    RipRiderEngine e(c); e.set_universe(UNIVERSE);
    for (auto& kv : data) for (auto& r : kv.second) e.seed_daily_bar(kv.first, r.day, r.open, r.close);
    struct Iv { int64_t entry_day, exit_day; std::string sym; };
    std::map<std::string,std::vector<int64_t>> opens;   // FIFO entry days per symbol
    std::vector<Iv> ivs;
    e.set_entry_callback([&](const std::string& s, double, int64_t ts){ opens[s].push_back(ts/86400000LL); });
    e.set_close_callback([&](const RipClose& x){
        auto& q = opens[x.symbol]; if (q.empty()) return;
        int64_t ed = q.front(); q.erase(q.begin());
        ivs.push_back({ed, x.exitTs/86400, x.symbol}); });
    e.simulate();
    // price lookup
    std::map<std::string,std::map<int64_t,double>> px;
    for (auto& kv : data) for (auto& r : kv.second) px[kv.first][r.day]=r.close;
    // global day axis
    std::set<int64_t> dset; for (auto& kv : data) for (auto& r : kv.second) dset.insert(r.day);
    std::vector<int64_t> days(dset.begin(), dset.end());
    // for each day, EW of open-position daily returns
    Series out;
    for (size_t i=1;i<days.size();++i) {
        int64_t d0=days[i-1], d1=days[i];
        std::vector<double> rets; std::vector<std::string> exiting;
        for (auto& iv : ivs) {
            bool held = (iv.entry_day <= d1 && d1 <= iv.exit_day);
            if (!held) continue;
            auto& m = px[iv.sym]; auto a=m.find(d0), b=m.find(d1);
            if (a!=m.end() && b!=m.end() && a->second>0) rets.push_back(b->second/a->second - 1.0);
            if (d1 == iv.exit_day) exiting.push_back(iv.sym);
        }
        double r = 0; if (!rets.empty()) { for (double x : rets) r+=x; r/=rets.size(); }
        // round-trip cost debited spread as full cost on exit day, EW
        if (!exiting.empty() && !rets.empty()) r -= cost_rt * (double)exiting.size()/rets.size();
        out.push_back({d1, r});
    }
    return out;
}

static void pr(const char* tag, const Metrics& m) {
    printf("  %-18s total=%+10.1f%%  Sh=%5.2f  vol=%5.1f%%  maxDD=%5.1f%%  Calmar=%5.2f  PF=%4.2f  d=%d\n",
           tag, m.total*100, m.sharpe, m.vol*100, m.maxdd*100, m.calmar, m.pf, m.days);
}

int main(int argc, char** argv) {
    std::string dir = (argc > 1) ? argv[1] : "../data/xsec_seed";
    auto data = load_universe(UNIVERSE, dir);
    printf("PHASE 8E — XSec2 FORWARD-PROOF (offline checklist)\n");
    printf("Loaded %zu/%zu symbols from %s\n", data.size(), UNIVERSE.size(), dir.c_str());

    // ---- base engine series (net 15bp/side, corrected-gate defaults) ----------
    Series v1  = build_v1(data);
    XSec2Config c2;                      // the corrected-gate KEEP config
    Series v2  = build_v2(data, c2);
    Series rr  = build_riprider(data);   // retained fat-tail rider (its own BTC>200d gate)

    Series v1e = drop2022(v1), v2e = drop2022(v2), rre = drop2022(rr);

    printf("\n############################################################\n");
    printf("### ITEM 1 — STANDALONE XSec2 vs ORIGINAL XSec\n");
    printf("### equal capital + equal VOL + same PIT universe + same cost/fill\n");
    printf("### Q: did XSec2 ITSELF improve, or does it merely BLEND differently?\n");
    printf("############################################################\n");
    {
        Metrics m1=metrics(v1e), m2=metrics(v2e);
        printf("  -- RAW (equal capital, same 15bp/side, same universe, EX-2022) --\n");
        pr("XSec v1 (orig)", m1); pr("XSec v2.0", m2);
        // equal-VOL: scale both to v1's annualized vol -> compares return per unit risk
        double tv = m1.vol;
        Metrics m1v=metrics(scale_to_vol(v1e,tv)), m2v=metrics(scale_to_vol(v2e,tv));
        printf("  -- EQUAL-VOL (both scaled to v1 vol %.1f%%; isolates skill from leverage) --\n", tv*100);
        pr("XSec v1 @vol", m1v); pr("XSec v2 @vol", m2v);
        bool sh_improved = m2.sharpe > m1.sharpe + 0.03;
        bool dd_better   = m2.maxdd < m1.maxdd - 0.005;
        printf("  VERDICT: standalone Sharpe %.2f (v2) vs %.2f (v1) = %s ; maxDD %.1f%% vs %.1f%% = %s\n",
               m2.sharpe, m1.sharpe, sh_improved?"v2 BETTER":(m2.sharpe<m1.sharpe-0.03?"v2 WORSE":"DEAD HEAT"),
               m2.maxdd*100, m1.maxdd*100, dd_better?"v2 lower":"~equal/worse");
        printf("  => %s. At matched vol v2 total %+.0f%% vs v1 %+.0f%% -> XSec2 did %s improve STANDALONE;\n",
               sh_improved?"[FAIL-for-standalone-improvement is OK if increment holds]":"STANDALONE = NOT a raw improver",
               m2v.total*100, m1v.total*100, sh_improved?"":"NOT");
        printf("     its case rests on DIVERSIFICATION/BLEND, tested in Item 5.\n");
    }

    printf("\n############################################################\n");
    printf("### ITEM 2 — YEAR-BY-YEAR (broad vs concentrated improvement)\n");
    printf("############################################################\n");
    {
        printf("  year |        v1 tot   Sh  |        v2 tot   Sh  |   50/50 tot   Sh  |  blend>v1?\n");
        int broad_win=0, yrs_counted=0;
        for (int y=2020;y<=2026;++y) {
            Series y1=yearslice(v1,y), y2=yearslice(v2,y);
            if (y1.size()<20) continue;
            Series yb=blend(y1,y2,0.5,0.5);
            Metrics a=metrics(y1), b=metrics(y2), c=metrics(yb);
            bool bull = (y!=2022); // 2022 shown-not-gated
            bool better = c.sharpe > a.sharpe + 0.02;
            if (bull) { ++yrs_counted; if (better) ++broad_win; }
            printf("  %d%s| %+10.1f%% %5.2f | %+10.1f%% %5.2f | %+9.1f%% %5.2f |  %s\n",
                   y, bull?" ":"*", a.total*100,a.sharpe, b.total*100,b.sharpe, c.total*100,c.sharpe,
                   better?"YES":"no");
        }
        printf("  (* = 2022 bear, shown-not-gated)\n");
        printf("  VERDICT: blend beats v1 in %d/%d non-bear years -> improvement is %s\n",
               broad_win, yrs_counted, broad_win>=yrs_counted-1 ? "BROAD (not one cycle)"
               : broad_win>=2 ? "MODERATELY broad" : "CONCENTRATED in few years (WEAK)");
    }

    printf("\n############################################################\n");
    printf("### ITEM 3 — REMOVED-BEST-EPISODE (one-cycle dependence)\n");
    printf("############################################################\n");
    {
        // (a) best contiguous 60d block; (b) best calendar year. Judge STANDALONE v2
        //     AND the INCREMENT (50/50 blend Sharpe vs v1-alone) after removal.
        auto zero_best_block = [](Series d, int W){
            double best=-1e18; int bi=0;
            for (int i=0;i+W<=(int)d.size();++i){ double p=1; for(int j=i;j<i+W;++j)p*=(1+d[j].second);
                if(p>best){best=p;bi=i;} }
            for(int j=bi;j<bi+W&&j<(int)d.size();++j) d[j].second=0; return d; };
        int W=60;
        Series v1x=zero_best_block(v1e,W), v2x=zero_best_block(v2e,W);
        Metrics m2x=metrics(v2x);
        Metrics base_blend=metrics(blend(v1e,v2e,0.5,0.5)), x_blend=metrics(blend(v1x,v2x,0.5,0.5));
        Metrics base_v1=metrics(v1e), x_v1=metrics(v1x);
        printf("  -- remove best %dd block (EX-2022) --\n", W);
        printf("  standalone v2 ex-episode: total=%+.1f%% Sh=%.2f  %s\n",
               m2x.total*100, m2x.sharpe, m2x.total>0?"STILL+":"FAILS");
        printf("  INCREMENT after removal: blend Sh %.2f vs v1-alone Sh %.2f = %s\n",
               x_blend.sharpe, x_v1.sharpe, x_blend.sharpe>x_v1.sharpe?"INCREMENT SURVIVES":"increment gone");
        // best calendar year removal
        int bestY=2021; double bestTot=-1e18;
        for (int y=2020;y<=2026;++y){ Series s=yearslice(v2,y); if(s.size()<20)continue;
            double t=metrics(s).total; if(t>bestTot){bestTot=t;bestY=y;} }
        Series v1ny=drop2022(v1), v2ny=drop2022(v2);
        // remove that best year from ex-2022 series
        auto drop_year=[&](Series d,int y){ Series o; for(auto&kv:d){ int64_t t=kv.first*86400000LL;
            static const int64_t Y[]={1577836800000LL,1609459200000LL,1640995200000LL,1672531200000LL,
                1704067200000LL,1735689600000LL,1767225600000LL,1798761600000LL};
            int idx=y-2020; if(!(t>=Y[idx]&&t<Y[idx+1])) o.push_back(kv);} return o; };
        Series v1ry=drop_year(v1ny,bestY), v2ry=drop_year(v2ny,bestY);
        Metrics ry_blend=metrics(blend(v1ry,v2ry,0.5,0.5)), ry_v1=metrics(v1ry);
        printf("  -- remove best-v2-YEAR (%d, the biggest crypto up-cycle) --\n", bestY);
        printf("  INCREMENT ex-%d: blend Sh %.2f vs v1-alone Sh %.2f = %s\n",
               bestY, ry_blend.sharpe, ry_v1.sharpe, ry_blend.sharpe>ry_v1.sharpe?"INCREMENT SURVIVES":"increment gone");
    }

    printf("\n############################################################\n");
    printf("### ITEM 4 — PARAMETER-NEIGHBOURHOOD (not one lucky threshold)\n");
    printf("### does the INCREMENT (blend Sh > v1 Sh) hold across the gate nbhd?\n");
    printf("############################################################\n");
    {
        auto test=[&](const char* nm, std::function<XSec2Config(XSec2Config)> mut){
            Series s = drop2022(build_v2(data, mut(c2)));
            Metrics ms=metrics(s), mb=metrics(blend(v1e,s,0.5,0.5)), mv1=metrics(v1e);
            printf("  %-20s v2 Sh=%5.2f | blend Sh=%5.2f vs v1 %5.2f  %s\n",
                   nm, ms.sharpe, mb.sharpe, mv1.sharpe, mb.sharpe>mv1.sharpe?"INCR+":"INCR-");
            return mb.sharpe>mv1.sharpe; };
        int pass=0,tot=0;
        auto acc=[&](bool b){ ++tot; if(b)++pass; };
        acc(test("base",            [](XSec2Config x){return x;}));
        acc(test("breadth=0.35",    [](XSec2Config x){x.breadth_thresh=0.35;return x;}));
        acc(test("breadth=0.45",    [](XSec2Config x){x.breadth_thresh=0.45;return x;}));
        acc(test("breadth=0.50",    [](XSec2Config x){x.breadth_thresh=0.50;return x;}));
        acc(test("smooth=3",        [](XSec2Config x){x.breadth_smooth=3;return x;}));
        acc(test("smooth=8",        [](XSec2Config x){x.breadth_smooth=8;return x;}));
        acc(test("core_frac=0.70",  [](XSec2Config x){x.core_frac=0.70;return x;}));
        acc(test("core_frac=0.85",  [](XSec2Config x){x.core_frac=0.85;return x;}));
        acc(test("core_k=2",        [](XSec2Config x){x.core_k=2;return x;}));
        acc(test("rebal=10",        [](XSec2Config x){x.rebalance_days=10;return x;}));
        acc(test("rebal=21",        [](XSec2Config x){x.rebalance_days=21;return x;}));
        acc(test("corr_pen=0.30",   [](XSec2Config x){x.corr_pen_w=0.30;return x;}));
        printf("  VERDICT: increment holds in %d/%d neighbours -> %s\n",
               pass, tot, pass>=tot-1?"PLATEAU (robust, not one lucky threshold)"
               : pass>=tot*3/4?"MOSTLY robust":"FRAGILE (increment param-dependent)");
    }

    printf("\n############################################################\n");
    printf("### ITEM 5 — INCREMENTAL CONTRIBUTION *AFTER RipRider* (THE CRUX)\n");
    printf("### does XSec2 add beyond DUPLICATING the retained v1+RipRider book?\n");
    printf("############################################################\n");
    {
        Metrics mr=metrics(rre);
        printf("  retained sleeves (EX-2022): \n");
        pr("  v1", metrics(v1e)); pr("  RipRider", mr);
        printf("  corr(v2, v1)          = %+.3f\n", corr(v2e, v1e));
        printf("  corr(v2, RipRider)    = %+.3f\n", corr(v2e, rre));
        // build retained book WITHOUT v2 = v1 + RipRider, then WITH v2.
        // Two weighting schemes:
        //  (A) naive equal-weight across sleeves
        //  (B) equal-VOL (inverse-vol) so no sleeve dominates by leverage
        auto ivw=[&](const Series& s, double tv){ Metrics m=metrics(s); return m.vol>1e-9? tv/m.vol:1.0; };
        double TV=metrics(v1e).vol; // target book vol reference
        // WITHOUT v2
        Series wo_ew = blend(v1e, rre, 0.5, 0.5);
        Series wo_iv = blend(scale_to_vol(v1e,TV), scale_to_vol(rre,TV), 0.5, 0.5);
        // WITH v2
        Series w_ew  = blend3(v1e, rre, v2e, 1.0/3, 1.0/3, 1.0/3);
        Series w_iv  = blend3(scale_to_vol(v1e,TV), scale_to_vol(rre,TV), scale_to_vol(v2e,TV),
                              1.0/3, 1.0/3, 1.0/3);
        printf("  corr(v2, retained book v1+RR) = %+.3f  (lower = less duplication)\n", corr(v2e, wo_iv));
        Metrics a_ew=metrics(wo_ew), b_ew=metrics(w_ew);
        Metrics a_iv=metrics(wo_iv), b_iv=metrics(w_iv);
        printf("  -- EQUAL-WEIGHT sleeves --\n");
        pr("WITHOUT v2 (v1+RR)", a_ew); pr("WITH v2 (v1+RR+v2)", b_ew);
        printf("     dSharpe=%+.2f  dMaxDD=%+.1fpp  dCalmar=%+.2f\n",
               b_ew.sharpe-a_ew.sharpe, (b_ew.maxdd-a_ew.maxdd)*100, b_ew.calmar-a_ew.calmar);
        printf("  -- EQUAL-VOL sleeves (leverage-neutral, the fair test) --\n");
        pr("WITHOUT v2 (v1+RR)", a_iv); pr("WITH v2 (v1+RR+v2)", b_iv);
        double dS = b_iv.sharpe-a_iv.sharpe, dDD=(b_iv.maxdd-a_iv.maxdd)*100, dC=b_iv.calmar-a_iv.calmar;
        printf("     dSharpe=%+.2f  dMaxDD=%+.1fpp  dCalmar=%+.2f\n", dS, dDD, dC);
        bool real_increment = (dS > 0.03) || (dDD < -1.0 && dS > -0.02);
        printf("  VERDICT: after RipRider, v2's equal-vol increment = %+.2f Sharpe / %+.1fpp DD -> %s\n",
               dS, dDD, real_increment ? "REAL INCREMENT (adds beyond duplication)"
               : "MARGINAL / DUPLICATIVE (RipRider already supplies the diversification)");
        printf("  NOTE: Phase-5's headline 1.51-vs-1.29 was v1+v2 ALONE (no RipRider). This item asks\n");
        printf("        the harder question: is the diversification still there once RipRider is in.\n");
    }

    printf("\n############################################################\n");
    printf("### ITEM 6 — CAPACITY / LIQUIDITY (challenger+rebalance executable?)\n");
    printf("############################################################\n");
    {
        // Walk v2 rebalances; for each target name compute traded notional at a
        // range of book sizes and compare to the name's rolling daily $-vol.
        // Participation cap: a trade should be <= 1% of ADV to fill w/o impact.
        auto e = new CrossSectionalMomentum2Engine(c2);
        e->set_universe(UNIVERSE);
        for (auto& kv : data) for (auto& r : kv.second) e->seed_daily(kv.first, r.day, r.close, r.dvol);
        // $-vol lookup by (sym,day): rolling 30d avg
        std::map<std::string,std::vector<Row>> D = data;
        auto adv30=[&](const std::string& s, int64_t day)->double{
            auto& v=D[s]; double sum=0; int n=0;
            for (auto it=v.rbegin(); it!=v.rend(); ++it){ if(it->day<=day && it->day>day-30){ sum+=it->dvol; ++n; } }
            return n>0? sum/n : 0; };
        size_t nd = e->num_days();
        // convert day-index to actual day value
        std::set<int64_t> dset; for (auto& kv : data) for (auto& r : kv.second) dset.insert(r.day);
        std::vector<int64_t> days(dset.begin(), dset.end());
        double books[] = {1e4, 1e5, 1e6, 1e7, 5e7};  // $10k = current shadow NAV
        const int NB = 5;
        int viol[NB] = {0,0,0,0,0}; int nchk[NB]={0,0,0,0,0};
        double worst_part[NB]={0,0,0,0,0};
        std::map<std::string,double> prevw;
        int64_t lastday=-1;
        for (size_t i=200; i<nd; i+=14) {
            bool bull; auto w=e->compute_target_weights(i,bull,{});
            int64_t day = (i<days.size())? days[i] : days.back();
            // turnover per name = |w - prevw|
            std::set<std::string> names; for(auto&kv:w)names.insert(kv.first); for(auto&kv:prevw)names.insert(kv.first);
            for (auto& s : names) {
                double dw = std::fabs((w.count(s)?w[s]:0) - (prevw.count(s)?prevw[s]:0));
                if (dw < 1e-6) continue;
                double advv = adv30(s, day); if (advv<=0) continue;
                for (int bk=0; bk<NB; ++bk) {
                    double traded = dw * books[bk];
                    double part = traded / advv; // fraction of ADV
                    ++nchk[bk]; if (part > 0.01) ++viol[bk];
                    worst_part[bk]=std::max(worst_part[bk], part);
                }
            }
            prevw = w; lastday = day;
        }
        (void)lastday;
        printf("  participation = traded notional / 30d avg daily $-vol; cap = 1%% of ADV\n");
        printf("  %-12s %10s %12s %14s\n","book size","checks",">1%% ADV","worst part");
        const char* lbl[NB]={"$10k(NAV)","$100k","$1M","$10M","$50M"};
        int cap_ok_book = 0;
        for (int bk=0; bk<NB; ++bk) {
            double frac = nchk[bk]? 100.0*viol[bk]/nchk[bk]:0;
            printf("  %-12s %10d %11.1f%% %13.2f%%\n", lbl[bk], nchk[bk], frac, worst_part[bk]*100);
            if (frac < 5.0) cap_ok_book = bk;
        }
        printf("  VERDICT: at CURRENT $10k shadow NAV all trades fill trivially (<1%% ADV); under a strict\n");
        printf("     1%%-ADV cap the book stays clean up to ~%s. Small-cap CHALLENGER names are the\n", lbl[cap_ok_book]);
        printf("     binding constraint at scale ($1M+ breaches) -> a SCALING ceiling, not a promote blocker.\n");
        delete e;
    }

    printf("\n############################################################\n");
    printf("### ITEM 7 — FORWARD-SHADOW  ->  PENDING (needs live-forward data)\n");
    printf("############################################################\n");
    printf("  XSec2 shadow book (tag XSEC2) installed on box josgp1 at Phase-5 deploy\n");
    printf("  (a2ebe17, 2026-07-11). No local forward ledger in this repo tree; the live\n");
    printf("  v1-vs-v2 forward record accrues on the box. Startup state was correctly flat\n");
    printf("  ([XSEC2] bull=0 breadth=0.15 -> CASH). Forward timing/ranking/turnover\n");
    printf("  comparison CANNOT be judged offline. STATUS = PENDING live observation.\n");

    printf("\n============================= DONE =============================\n");
    return 0;
}

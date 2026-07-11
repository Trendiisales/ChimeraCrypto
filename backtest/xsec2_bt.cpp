// ============================================================================
// xsec2_bt.cpp — XSec 1 (live) vs XSec 2.0 (Phase-5) faithful daily backtest +
// the FULL VALIDATION STANDARD (CRYPTO_REVIEW_BACKLOG.md). Long-only spot, NO
// 200DMA in v2.0. Reads the committed daily warm-seed CSVs in data/xsec_seed/.
//
//   build:  g++ -std=c++20 -O2 -I../include xsec2_bt.cpp -o xsec2_bt
//   run:    ./xsec2_bt            (from backtest/, needs ../data/xsec_seed)
// ============================================================================
#include "core/CrossSectionalMomentumEngine.hpp"
#include "core/CrossSectionalMomentum2Engine.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <algorithm>

using namespace chimera;

// ---- data --------------------------------------------------------------------
struct Row { int64_t day; double close, dvol; };
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
            double o = strtod(e+1, &e);        // open
            double h = strtod(e+1, &e);        // high
            double l = strtod(e+1, &e);        // low
            double c = strtod(e+1, &e);        // close
            double vol = strtod(e+1, &e);      // base volume
            (void)o;(void)h;(void)l;
            if (c > 0) out[s].push_back({ ts/86400000LL, c, c*vol });
        }
        fclose(f);
    }
    return out;
}

// ---- metrics -----------------------------------------------------------------
struct Metrics {
    double total = 0, sum = 0, sharpe = 0, maxdd = 0, pf = 0;
    int    days = 0, active = 0;
};
static Metrics metrics(const std::vector<std::pair<int64_t,double>>& d) {
    Metrics m; m.days = (int)d.size();
    if (d.empty()) return m;
    double eq = 1.0, peak = 1.0, mean = 0, gp = 0, gl = 0;
    for (auto& kv : d) {
        double r = kv.second; m.sum += r; mean += r;
        if (std::fabs(r) > 1e-12) m.active++;
        if (r > 0) gp += r; else gl += -r;
        eq *= (1.0 + r); peak = std::max(peak, eq);
        m.maxdd = std::max(m.maxdd, (peak - eq)/peak);
    }
    m.total = eq - 1.0; mean /= d.size();
    double var = 0; for (auto& kv : d) { double x = kv.second - mean; var += x*x; }
    var /= d.size();
    double sd = var > 0 ? std::sqrt(var) : 0;
    m.sharpe = sd > 0 ? mean/sd*std::sqrt(365.0) : 0;
    m.pf = gl > 0 ? gp/gl : (gp > 0 ? 999.0 : 0.0);
    return m;
}
// slice by ms window [lo,hi)
static std::vector<std::pair<int64_t,double>> window(const std::vector<std::pair<int64_t,double>>& d,
                                                     int64_t lo_ms, int64_t hi_ms) {
    std::vector<std::pair<int64_t,double>> o;
    for (auto& kv : d) { int64_t t = kv.first*86400000LL; if (t >= lo_ms && t < hi_ms) o.push_back(kv); }
    return o;
}
static std::vector<std::pair<int64_t,double>> half(const std::vector<std::pair<int64_t,double>>& d, int which) {
    std::vector<std::pair<int64_t,double>> o; size_t n = d.size();
    for (size_t i = 0; i < n; ++i) if ((which==0) == (i < n/2)) o.push_back(d[i]);
    return o;
}

// build both engines seeded identically
static const std::vector<std::string> UNIVERSE = {
    "AAVE","ADA","ALGO","APE","AR","ATOM","AVAX","AXS","BAT","BCH","BNB","BTC",
    "CAKE","CHZ","COMP","CRV","DASH","DOGE","DOT","EGLD","ENJ","ETC","ETH","FET",
    "FIL","GALA","GRT","HBAR","ICP","INJ","KSM","LDO","LINK","LTC","MANA","NEAR",
    "NEO","ONE","QTUM","RUNE","SAND","SHIB","SNX","SOL","SUSHI","THETA","TRX",
    "UNI","VET","XLM","XRP","XTZ","ZEC","ZIL" };

int main(int argc, char** argv) {
    std::string dir = (argc > 1) ? argv[1] : "../data/xsec_seed";
    auto data = load_universe(UNIVERSE, dir);
    printf("Loaded %zu/%zu symbols from %s\n", data.size(), UNIVERSE.size(), dir.c_str());

    // ---- v1 (live logic — dual sleeve reproduced as its two configs merged) --
    // The live book runs a BTC-gated 60% sleeve + breadth-gated 40% sleeve. To
    // compare like-for-like we build the SAME two sleeves and 60/40 blend them,
    // exactly as src/main.cpp wires XSEC-BTC + XSEC-BR.
    XSecConfig v1b; v1b.gate_mode = 0;                          // BTC>200d
    XSecConfig v1r; v1r.gate_mode = 1; v1r.breadth_thresh = 0.65;// breadth
    auto build_v1 = [&](const XSecConfig& c){ auto* e = new CrossSectionalMomentumEngine(c);
        e->set_universe(UNIVERSE);
        for (auto& kv : data) for (auto& r : kv.second) e->seed_daily_close(kv.first, r.day, r.close);
        return e; };
    // ---- v2.0 ---------------------------------------------------------------
    auto build_v2 = [&](XSec2Config c){ auto* e = new CrossSectionalMomentum2Engine(c);
        e->set_universe(UNIVERSE);
        for (auto& kv : data) for (auto& r : kv.second) e->seed_daily(kv.first, r.day, r.close, r.dvol);
        return e; };

    auto blend = [](const std::vector<std::pair<int64_t,double>>& a,
                    const std::vector<std::pair<int64_t,double>>& b, double wa, double wb){
        std::map<int64_t,double> m; for (auto& kv : a) m[kv.first] += wa*kv.second;
        for (auto& kv : b) m[kv.first] += wb*kv.second;
        std::vector<std::pair<int64_t,double>> o(m.begin(), m.end()); return o; };

    auto* e1b = build_v1(v1b); auto* e1r = build_v1(v1r);
    auto v1_daily = blend(e1b->simulate(), e1r->simulate(), 0.6, 0.4);

    XSec2Config c2;
    auto* e2 = build_v2(c2);
    auto v2_daily = e2->simulate();

    // cost-scan for turnover (arithmetic drag / cost-rate = 2-sided turnover sum)
    auto turnover = [&](auto build, auto cfg_setter){
        auto cfg0 = cfg_setter(0.0); auto cfgc = cfg_setter(15.0);
        auto* a = build(cfg0); auto* b = build(cfgc);
        double s0 = metrics(a->simulate()).sum, sc = metrics(b->simulate()).sum;
        delete a; delete b;
        return (s0 - sc) / (15.0/10000.0);   // total two-sided turnover fraction
    };
    // v1 turnover (blend of two sleeves)
    double v1_turn = 0;
    { auto mk = [&](double bps){ XSecConfig x=v1b; x.cost_bps=bps; return x; };
      auto mkr= [&](double bps){ XSecConfig x=v1r; x.cost_bps=bps; return x; };
      auto* a0=build_v1(mk(0)); auto* ac=build_v1(mk(15)); auto* r0=build_v1(mkr(0)); auto* rc=build_v1(mkr(15));
      double s0 = metrics(blend(a0->simulate(),r0->simulate(),0.6,0.4)).sum;
      double sc = metrics(blend(ac->simulate(),rc->simulate(),0.6,0.4)).sum;
      v1_turn = (s0-sc)/(15.0/10000.0); }
    double v2_turn = turnover(build_v2, [&](double bps){ XSec2Config x=c2; x.cost_bps=bps; return x; });

    auto pr = [&](const char* tag, const Metrics& m){
        printf("  %-10s total=%+9.1f%%  Sharpe=%5.2f  maxDD=%5.1f%%  PF=%4.2f  days=%d active=%d\n",
               tag, m.total*100, m.sharpe, m.maxdd*100, m.pf, m.days, m.active); };

    Metrics m1 = metrics(v1_daily), m2 = metrics(v2_daily);
    printf("\n================= HEADLINE (net 15bp/side) =================\n");
    pr("XSec v1", m1); pr("XSec v2.0", m2);
    printf("  turnover(2-sided, whole period): v1=%.1fx  v2=%.1fx\n", v1_turn, v2_turn);

    // ---- EX-2022 re-judgment (long-only sits out bears; 2022 shown-not-gated) --
    // Standing rule (feedback-crypto-omit-2022-longonly): a long-only spot book
    // cannot trade a bear, so 2022 is NOT a pass/fail gate — remove the 2022 window
    // and re-judge on the non-bear span. (2022 bleed is still shown above.)
    printf("\n================= EX-2022 (long-only, bear NOT gated) =================\n");
    auto drop = [&](const std::vector<std::pair<int64_t,double>>& d, int64_t lo, int64_t hi){
        std::vector<std::pair<int64_t,double>> o; for (auto& kv : d){ int64_t t=kv.first*86400000LL;
            if (!(t>=lo && t<hi)) o.push_back(kv); } return o; };
    int64_t X22lo=1640995200000LL, X22hi=1672531200000LL; // 2022 window
    auto v1_ex = drop(v1_daily, X22lo, X22hi);
    auto v2_ex = drop(v2_daily, X22lo, X22hi);
    Metrics m1e = metrics(v1_ex), m2e = metrics(v2_ex);
    pr("v1 ex-2022", m1e); pr("v2.0 ex-2022", m2e);
    Metrics x1a=metrics(half(v1_ex,0)), x1b=metrics(half(v1_ex,1));
    Metrics x2a=metrics(half(v2_ex,0)), x2b=metrics(half(v2_ex,1));
    printf("  v1  ex22 WF: H1 %+8.1f%% Sh%5.2f | H2 %+8.1f%% Sh%5.2f  %s\n",
           x1a.total*100,x1a.sharpe,x1b.total*100,x1b.sharpe,(x1a.total>0&&x1b.total>0)?"BOTH+":"FAIL");
    printf("  v2  ex22 WF: H1 %+8.1f%% Sh%5.2f | H2 %+8.1f%% Sh%5.2f  %s\n",
           x2a.total*100,x2a.sharpe,x2b.total*100,x2b.sharpe,(x2a.total>0&&x2b.total>0)?"BOTH+":"FAIL");
    { auto comb = blend(v1_ex, v2_ex, 0.5, 0.5); Metrics mc = metrics(comb);
      pr("50/50 ex-2022", mc);
      printf("  -> ex-2022 combined Sharpe %.2f vs v1-alone %.2f : v2 %s the combined book\n",
             mc.sharpe, m1e.sharpe, (mc.sharpe>m1e.sharpe)?"IMPROVES":"does NOT improve"); }

    // ---- WF both halves ----
    printf("\n================= WALK-FORWARD (both halves) =================\n");
    Metrics h1a=metrics(half(v1_daily,0)), h1b=metrics(half(v1_daily,1));
    Metrics h2a=metrics(half(v2_daily,0)), h2b=metrics(half(v2_daily,1));
    printf("  v1  H1 total=%+8.1f%% Sh=%5.2f | H2 total=%+8.1f%% Sh=%5.2f  %s\n",
           h1a.total*100,h1a.sharpe,h1b.total*100,h1b.sharpe,(h1a.total>0&&h1b.total>0)?"BOTH+":"FAIL");
    printf("  v2  H1 total=%+8.1f%% Sh=%5.2f | H2 total=%+8.1f%% Sh=%5.2f  %s\n",
           h2a.total*100,h2a.sharpe,h2b.total*100,h2b.sharpe,(h2a.total>0&&h2b.total>0)?"BOTH+":"FAIL");

    // ---- bear-period behaviour ----
    printf("\n================= BEAR-PERIOD behaviour =================\n");
    int64_t B22lo=1640995200000LL, B22hi=1672531200000LL; // 2022
    int64_t B25lo=1735689600000LL, B25hi=1767225600000LL; // 2025 (alt-bear holdout)
    Metrics v1b22=metrics(window(v1_daily,B22lo,B22hi)), v2b22=metrics(window(v2_daily,B22lo,B22hi));
    Metrics v1b25=metrics(window(v1_daily,B25lo,B25hi)), v2b25=metrics(window(v2_daily,B25lo,B25hi));
    printf("  2022 bear: v1 total=%+7.1f%% (active %d/%d) | v2 total=%+7.1f%% (active %d/%d)\n",
           v1b22.total*100,v1b22.active,v1b22.days, v2b22.total*100,v2b22.active,v2b22.days);
    printf("  2025 bear: v1 total=%+7.1f%% (active %d/%d) | v2 total=%+7.1f%% (active %d/%d)\n",
           v1b25.total*100,v1b25.active,v1b25.days, v2b25.total*100,v2b25.active,v2b25.days);

    // ---- 2x cost stress ----
    printf("\n================= 2x-COST STRESS (30bp/side) =================\n");
    { XSec2Config x=c2; x.cost_bps=30.0; auto* e=build_v2(x); Metrics m=metrics(e->simulate());
      XSecConfig y=v1b; y.cost_bps=30.0; XSecConfig yr=v1r; yr.cost_bps=30.0;
      auto* a=build_v1(y); auto* r=build_v1(yr); Metrics mv1=metrics(blend(a->simulate(),r->simulate(),0.6,0.4));
      printf("  v1 total=%+8.1f%% Sh=%5.2f %s | v2 total=%+8.1f%% Sh=%5.2f %s\n",
             mv1.total*100,mv1.sharpe,mv1.total>0?"+":"FAIL", m.total*100,m.sharpe,m.total>0?"+":"FAIL"); }

    // ---- parameter-neighbourhood plateau (v2) ----
    printf("\n================= PARAM-NEIGHBOURHOOD PLATEAU (v2) =================\n");
    auto scan = [&](const char* name, std::function<XSec2Config(XSec2Config)> mut){
        XSec2Config x = mut(c2); auto* e = build_v2(x); Metrics m = metrics(e->simulate()); delete e;
        printf("  %-22s total=%+8.1f%%  Sharpe=%5.2f  %s\n", name, m.total*100, m.sharpe, m.total>0?"+":"NEG"); };
    scan("base",           [](XSec2Config x){ return x; });
    scan("core_frac=0.70", [](XSec2Config x){ x.core_frac=0.70; return x; });
    scan("core_frac=0.85", [](XSec2Config x){ x.core_frac=0.85; return x; });
    scan("breadth=0.35",   [](XSec2Config x){ x.breadth_thresh=0.35; return x; });
    scan("breadth=0.45",   [](XSec2Config x){ x.breadth_thresh=0.45; return x; });
    scan("wt 25/35/25/15",[](XSec2Config x){ x.wt_short=0.25;x.wt_mid=0.35;x.wt_long=0.25;x.wt_accel=0.15; return x; });
    scan("wt 35/30/20/15",[](XSec2Config x){ x.wt_short=0.35;x.wt_mid=0.30;x.wt_long=0.20;x.wt_accel=0.15; return x; });
    scan("core_k=2",       [](XSec2Config x){ x.core_k=2; return x; });
    scan("corr_pen=0.30",  [](XSec2Config x){ x.corr_pen_w=0.30; return x; });
    scan("rebal_days=10",  [](XSec2Config x){ x.rebalance_days=10; return x; });

    // ---- ex-best-episode (remove best 30d contiguous block, v2) ----
    printf("\n================= EX-BEST-EPISODE (v2) =================\n");
    {
        auto d = v2_daily; int W=30; double best=-1e18; int bi=0;
        for (int i=0;i+W<=(int)d.size();++i){ double p=1; for(int j=i;j<i+W;++j)p*=(1+d[j].second);
            if (p>best){best=p;bi=i;} }
        auto d2=d; for(int j=bi;j<bi+W&&j<(int)d2.size();++j) d2[j].second=0;
        Metrics m=metrics(d2);
        printf("  best 30d block starts idx %d (x%.2f); ex-episode total=%+8.1f%% Sharpe=%5.2f %s\n",
               bi,best,m.total*100,m.sharpe,m.total>0?"STILL+":"FAILS");
    }

    // ---- random-entry control (v2 universe/count, random picks) ----
    printf("\n================= RANDOM-ENTRY CONTROL =================\n");
    {
        std::mt19937 rng(12345);
        // reconstruct v2 dense axis via a fresh engine to walk eligibility
        auto* e = build_v2(c2);
        // Monte-Carlo: at each 14d step pick 3 random eligible coins equal-weight.
        double best_rand=-1e18, worst_rand=1e18, mean_rand=0; int N=200; std::vector<double> draws;
        // We approximate using e's public eligibility on the shared day axis by
        // re-deriving returns from data. Build a day axis map.
        // Simple proxy: random long of 3 eligible names, rebalanced 14d, cost 15.
        std::vector<int64_t> days; { std::map<int64_t,int> seen;
            for (auto& kv : data) for (auto& r : kv.second) if(!seen.count(r.day)){seen[r.day]=1;days.push_back(r.day);} }
        std::sort(days.begin(),days.end());
        std::map<std::string,std::map<int64_t,double>> px;
        for (auto& kv : data) for (auto& r : kv.second) px[kv.first][r.day]=r.close;
        for (int it=0; it<N; ++it) {
            std::map<std::string,double> w; int64_t last=-1<<30; double eq=1,peak=1; double sum=0;
            for (size_t i=1;i<days.size();++i){ double ret=0;
                for(auto&kv:w){ auto&m=px[kv.first]; auto a=m.find(days[i-1]),b=m.find(days[i]);
                    if(a!=m.end()&&b!=m.end()&&a->second>0) ret+=kv.second*(b->second/a->second-1); }
                sum+=ret; eq*=(1+ret); peak=std::max(peak,eq);
                if(days[i]-last>=14){ last=days[i];
                    std::vector<std::string> el; for(auto&kv:px){ auto b=kv.second.find(days[i]);
                        // require 120d history + present
                        auto h=kv.second.find(days[i]-120); if(b!=kv.second.end()&&h!=kv.second.end()) el.push_back(kv.first); }
                    std::shuffle(el.begin(),el.end(),rng);
                    std::map<std::string,double> nw; for(int k=0;k<3&&k<(int)el.size();++k) nw[el[k]]=1.0/3;
                    double turn=0; std::set<std::string> all; for(auto&kv:w)all.insert(kv.first); for(auto&kv:nw)all.insert(kv.first);
                    for(auto&k:all) turn+=std::fabs((nw.count(k)?nw[k]:0)-(w.count(k)?w[k]:0));
                    sum-=turn*15.0/10000.0; w=nw; } }
            double tot=eq-1; draws.push_back(tot); mean_rand+=tot; best_rand=std::max(best_rand,tot); worst_rand=std::min(worst_rand,tot);
        }
        mean_rand/=N;
        int beat_cnt=0; for (double t : draws) if (m2.total > t) ++beat_cnt;
        double pct = 100.0*beat_cnt/std::max(1,(int)draws.size());
        // percentile rank of v2 among the random draws (the proper control test:
        // a real signal should beat the MEAN/median random pick, not the single
        // luckiest draw of N).
        printf("  random 3-coin long (N=%d): mean=%+.1f%%  best=%+.1f%%  worst=%+.1f%%\n",
               N, mean_rand*100, best_rand*100, worst_rand*100);
        printf("  v2.0 total=%+.1f%%  -> beats %.0f%% of random draws (mean x%.1f)  %s\n",
               m2.total*100, pct, mean_rand>-1? (1+m2.total)/(1+mean_rand):0.0,
               pct>=75.0?"PASS":"WEAK");
        delete e;
    }

    // ---- portfolio-increment: does v2 improve the COMBINED book vs v1-alone? --
    printf("\n================= PORTFOLIO-INCREMENT (v2 alongside v1) =================\n");
    {
        // daily-return correlation v1 vs v2 on the shared axis
        std::map<int64_t,double> mv1, mv2;
        for (auto& kv : v1_daily) mv1[kv.first]=kv.second;
        for (auto& kv : v2_daily) mv2[kv.first]=kv.second;
        std::vector<double> a,b; for (auto& kv : mv1){ auto it=mv2.find(kv.first); if(it!=mv2.end()){a.push_back(kv.second);b.push_back(it->second);} }
        double ma=0,mb=0; for(size_t i=0;i<a.size();++i){ma+=a[i];mb+=b[i];} ma/=a.size();mb/=b.size();
        double sab=0,saa=0,sbb=0; for(size_t i=0;i<a.size();++i){double da=a[i]-ma,db=b[i]-mb;sab+=da*db;saa+=da*da;sbb+=db*db;}
        double rho = (saa>0&&sbb>0)? sab/std::sqrt(saa*sbb):0;
        auto comb = blend(v1_daily, v2_daily, 0.5, 0.5);
        Metrics mc = metrics(comb);
        printf("  corr(v1,v2 daily) = %+.3f  (lower = more diversifying)\n", rho);
        pr("v1 alone",   m1);
        pr("v2 alone",   m2);
        pr("50/50 v1+v2",mc);
        printf("  -> combined Sharpe %.2f vs v1-alone %.2f : v2 %s the combined book\n",
               mc.sharpe, m1.sharpe, (mc.sharpe > m1.sharpe)?"IMPROVES":"does NOT improve");
    }

    // ---- capital utilization ----
    printf("\n================= CAPITAL UTILIZATION (v2) =================\n");
    {
        // walk the v2 axis: fraction of days deployed vs cash, avg #positions
        auto* e = build_v2(c2);
        size_t nd = e->num_days(); int cash_days=0, pos_sum=0, sample=0;
        for (size_t i=200; i<nd; i+=7) { bool bull; auto w=e->compute_target_weights(i,bull,{});
            int np=0; double dep=0; for(auto&kv:w){ if(kv.second>0){++np;dep+=kv.second;} }
            if (!bull || np==0) ++cash_days; else pos_sum+=np; ++sample; }
        printf("  sampled %d weekly points: cash %d (%.0f%%), avg #positions when deployed=%.2f\n",
               sample, cash_days, 100.0*cash_days/std::max(1,sample),
               pos_sum/(double)std::max(1,sample-cash_days));
        delete e;
    }

    printf("\n================= DONE =================\n");
    return 0;
}

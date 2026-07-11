// ============================================================================
// phase6b_twostage_bt.cpp — Item 28: TWO-STAGE (tranche) IGNITION ENTRY on the
// VALIDATED RipRider parent. Long-only spot, NO shorts, NO 200DMA.
//
// The parent (RipRiderEngine / crypto_momo_rider): ignition = close up >= ig_pct
// over lb_days -> ENTER at the next day's OPEN, RIDE to the regime flip (BTC<200d)
// or maxhold. The EXIT logic is the validated parent's and is NOT changed here.
//
// This tests whether SPLITTING the ENTRY into tranches, at IDENTICAL total capital
// per signal (1.0 unit budget; unfilled tranches = undeployed cash, no PnL) and
// with REAL fills off the daily O/H/L/C, improves net and/or fill quality:
//
//   IMM      immediate-only parent           : 100% @ open[i+1]
//   IMM_PB   immediate + pullback (expiring)  : 40% @ open + 60% @ open*(1-pb) if low<=limit within pb_exp
//   PB_ONLY  pullback-only (expiring)         : 100% @ open*(1-pb) if it pulls back, else NO trade
//   IMM_CONF immediate + confirmation         : 50% @ open + 50% @ open*(1+cf) if high>=lvl within cf_exp
//   SPLIT    immediate+pullback+confirmation  : 35% @ open + 40% pullback + 25% confirmation
//
// A tranche that never fills before its expiry (or before the position exits) is
// NEVER deployed. Position rides from the FIRST fill; blended entry = size-weighted
// mean of filled tranches; PnL over the DEPLOYED size only. Costs 10bp/side on
// deployed notional (split pays proportionally less). BTC 200d = regime gate.
//
//   build: g++ -std=c++20 -O2 -I../include phase6b_twostage_bt.cpp -o phase6b_twostage_bt
//   run:   ./phase6b_twostage_bt   (from backtest/, needs ../data/xsec_seed)
// ============================================================================
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

struct Row { long long day; double o,h,l,c; };
static const std::vector<std::string> UNIVERSE = {
    "AAVE","ADA","ALGO","APE","AR","ATOM","AVAX","AXS","BAT","BCH","BNB","BTC",
    "CAKE","CHZ","COMP","CRV","DASH","DOGE","DOT","EGLD","ENJ","ETC","ETH","FET",
    "FIL","GALA","GRT","HBAR","ICP","INJ","KSM","LDO","LINK","LTC","MANA","NEAR",
    "NEO","ONE","QTUM","RUNE","SAND","SHIB","SNX","SOL","SUSHI","THETA","TRX",
    "UNI","VET","XLM","XRP","XTZ","ZEC","ZIL" };
static std::map<std::string,std::vector<Row>> load(const std::string& dir){
    std::map<std::string,std::vector<Row>> out;
    for(auto&s:UNIVERSE){ std::string p=dir+"/"+s+"USDT_1d.csv"; FILE*f=fopen(p.c_str(),"r"); if(!f)continue;
        char ln[512]; bool first=true;
        while(fgets(ln,sizeof ln,f)){ if(first){first=false; if(!isdigit((unsigned char)ln[0]))continue;}
            char*e; long long ts=strtoll(ln,&e,10); if(e==ln)continue;
            double o=strtod(e+1,&e),h=strtod(e+1,&e),l=strtod(e+1,&e),c=strtod(e+1,&e);
            if(c>0) out[s].push_back({ts/86400000LL,o,h,l,c}); }
        fclose(f); }
    return out;
}

struct Cfg {
    double ig_pct=0.20; int lb_days=5; int maxhold=60;
    int btc_sma=200; double cost_side=0.0010; // 10bp/side
    // tranche weights
    double w_imm=1.0, w_pb=0.0, w_conf=0.0;
    double pb_depth=0.06; int pb_exp=5;
    double cf_up=0.05;   int cf_exp=5;
};

struct TradeRes { double ret_alloc; double ret_deploy; double deployed; double fill_vs_imm; bool bear_entry; long long day; };
struct Summary { int signals=0, trades=0; double sum_alloc=0,sum_deploy=0,dep_tot=0,fillimp=0;
                 int wins=0; std::vector<double> tr; std::vector<std::pair<long long,double>> series; };

// BTC regime helper
struct Btc { std::vector<long long> days; std::vector<double> close; std::map<long long,int> idx;
    double sma(long long d,int n) const {
        int pos = idx.count(d)?idx.at(d):-1; if(pos<0){ for(int k=(int)days.size()-1;k>=0;--k) if(days[k]<=d){pos=k;break;} }
        if(pos<n) return NAN; double s=0; int c=0; for(int j=pos-n;j<pos;++j){ if(!std::isnan(close[j])){s+=close[j];++c;} }
        return c>=n*0.8? s/c:NAN; }
    double px(long long d) const { int pos=idx.count(d)?idx.at(d):-1; if(pos<0){for(int k=(int)days.size()-1;k>=0;--k)if(days[k]<=d){pos=k;break;}} return pos>=0?close[pos]:NAN; }
    bool bull(long long d) const { double m=sma(d,200),c=px(d); if(std::isnan(m)||std::isnan(c))return true; return c>m; }
};

static void run(const char* name, const Cfg& cfg, std::map<std::string,std::vector<Row>>& data, const Btc& btc, Summary& S){
    for(auto&kv:data){ const std::string& s=kv.first; if(s=="BTC") continue; auto& R=kv.second;
        // index by day for O/H/L
        int n=(int)R.size(); if(n<cfg.lb_days+2) continue;
        int i=cfg.lb_days;
        while(i<n-1){
            // ignition on day i's close (ret over lb_days)
            double a=R[i-cfg.lb_days].c, b=R[i].c;
            if(a<=0||b<=0||(b/a-1.0)<cfg.ig_pct){ ++i; continue; }
            // regime at signal day i must be bull to enter (parent gate) — re-check at fill day
            long long sigday=R[i].day;
            // entry starts at i+1 open
            int e=i+1; if(e>=n){ break; }
            ++S.signals;
            // --- tranche setup ---
            double openpx=R[e].o;
            bool gate_ok = btc.bull(sigday);
            if(!gate_ok){ // signal in bear: parent would not enter (gate). count as signal, no trade.
                i=e; continue; }
            double pb_limit=openpx*(1.0-cfg.pb_depth);
            double cf_level=openpx*(1.0+cfg.cf_up);
            bool imm_f = cfg.w_imm>0, pb_f=false, cf_f=false;
            double imm_px = imm_f? openpx : 0;
            double pb_px=0, cf_px=0;
            bool active = imm_f;               // position active once immediate fills
            int first_fill = imm_f? e : -1;
            // ride day by day from e
            int j=e; int exit_j=-1; double exit_px=0;
            for(; j<n; ++j){
                long long d=R[j].day;
                // tranche fills (causal within expiry windows measured from e)
                if(cfg.w_pb>0 && !pb_f && (j-e)<=cfg.pb_exp){
                    double lo = (j==e)? std::min(R[j].l, openpx) : R[j].l; // on entry day, only post-open low modeled by day low
                    if(lo<=pb_limit){ pb_f=true; pb_px=pb_limit; if(!active){active=true; first_fill=j;} }
                }
                if(cfg.w_conf>0 && !cf_f && (j-e)<=cfg.cf_exp){
                    double hi = R[j].h;
                    if(hi>=cf_level){ cf_f=true; cf_px=cf_level; if(!active){active=true; first_fill=j;} }
                }
                if(!active) continue; // pullback-only not yet filled
                // exit checks (parent logic, UNCHANGED): regime flip or maxhold
                bool bear = !btc.bull(d);
                bool mh = (int)(d - R[first_fill].day) >= cfg.maxhold;
                if(bear || mh){ exit_j=j; exit_px=R[j].c; break; }
            }
            if(!active){ // no tranche ever filled (pullback never came) -> expired, no trade
                i=e; continue; }
            if(exit_j<0){ exit_j=n-1; exit_px=R[n-1].c; }
            // blended entry over filled tranches
            double wsum=0, wpx=0;
            if(imm_f){ wsum+=cfg.w_imm; wpx+=cfg.w_imm*imm_px; }
            if(pb_f){ wsum+=cfg.w_pb; wpx+=cfg.w_pb*pb_px; }
            if(cf_f){ wsum+=cfg.w_conf; wpx+=cfg.w_conf*cf_px; }
            if(wsum<=0){ i=e; continue; }
            double blended=wpx/wsum;
            double deployed=wsum;               // fraction of the 1.0 budget actually deployed
            double gross=exit_px/blended-1.0;
            double net=gross - 2.0*cfg.cost_side; // entry+exit cost on deployed notional
            double ret_alloc = deployed*net;    // per 1.0 allocated capital (identical budget)
            double ret_deploy = net;            // per deployed unit (fill/edge quality)
            ++S.trades; S.sum_alloc+=ret_alloc; S.sum_deploy+=ret_deploy; S.dep_tot+=deployed;
            S.fillimp += (openpx-blended)/openpx; // >0 = filled cheaper than immediate open
            if(ret_alloc>0) ++S.wins;
            S.tr.push_back(ret_alloc);
            S.series.push_back({R[exit_j].day, ret_alloc});
            i=exit_j+1;                          // next ignition after this trade closes
        }
    }
}

static double sharpe(const std::vector<double>& x){ if(x.size()<3)return 0; double m=0; for(double v:x)m+=v; m/=x.size();
    double s=0; for(double v:x){double d=v-m; s+=d*d;} s/=x.size(); s=s>0?std::sqrt(s):0; return s>0? m/s*std::sqrt(52.0):0; }
static double sum(const std::vector<double>& x){ double s=0; for(double v:x)s+=v; return s; }

int main(int argc,char**argv){
    std::string dir=(argc>1)?argv[1]:"../data/xsec_seed";
    auto data=load(dir);
    printf("Loaded %zu/%zu symbols\n", data.size(), UNIVERSE.size());
    // build BTC regime series
    Btc btc; { auto it=data.find("BTC"); if(it!=data.end()){ for(auto&r:it->second){ btc.idx[r.day]=(int)btc.days.size(); btc.days.push_back(r.day); btc.close.push_back(r.c);} } }
    if(btc.days.empty()){ printf("NO BTC data — abort\n"); return 1; }

    struct V{ const char* name; Cfg cfg; };
    std::vector<V> variants = {
        {"IMM (parent)",      [&]{Cfg c; c.w_imm=1.0; return c;}()},
        {"IMM_PB 40/60",      [&]{Cfg c; c.w_imm=0.4; c.w_pb=0.6; return c;}()},
        {"PB_ONLY 100",       [&]{Cfg c; c.w_imm=0.0; c.w_pb=1.0; return c;}()},
        {"IMM_CONF 50/50",    [&]{Cfg c; c.w_imm=0.5; c.w_conf=0.5; return c;}()},
        {"SPLIT 35/40/25",    [&]{Cfg c; c.w_imm=0.35; c.w_pb=0.40; c.w_conf=0.25; return c;}()},
        // robustness: shallower/deeper pullback + heavier-immediate split (fairness to the idea)
        {"IMM_PB sh(3%/8d)",  [&]{Cfg c; c.w_imm=0.4; c.w_pb=0.6; c.pb_depth=0.03; c.pb_exp=8; return c;}()},
        {"IMM_PB dp(10%/8d)", [&]{Cfg c; c.w_imm=0.4; c.w_pb=0.6; c.pb_depth=0.10; c.pb_exp=8; return c;}()},
        {"IMM_PB 70/30 sh",   [&]{Cfg c; c.w_imm=0.7; c.w_pb=0.3; c.pb_depth=0.03; c.pb_exp=8; return c;}()},
        {"SPLIT sh 40/40/20", [&]{Cfg c; c.w_imm=0.4; c.w_pb=0.4; c.w_conf=0.2; c.pb_depth=0.03; c.pb_exp=8; c.cf_up=0.03; c.cf_exp=8; return c;}()},
    };

    printf("\n%-18s %7s %7s %8s %9s %9s %8s %8s %8s %8s\n",
           "variant","signals","trades","dep.avg","net/alloc","net/dep","fillimp","winrate","Sharpe","WF(H1/H2)");
    printf("------------------------------------------------------------------------------------------------------------\n");
    Summary base;
    for(size_t vi=0; vi<variants.size(); ++vi){
        auto d2=data; Summary S; run(variants[vi].name, variants[vi].cfg, d2, btc, S);
        // WF halves by chronological trade order
        std::sort(S.series.begin(),S.series.end());
        std::vector<double> h1,h2; for(size_t k=0;k<S.series.size();++k){ (k<S.series.size()/2?h1:h2).push_back(S.series[k].second); }
        double alloc=sum(S.tr); double dep_avg=S.trades? S.dep_tot/S.trades:0;
        double netdep=S.trades? S.sum_deploy/S.trades:0;
        double fillimp=S.trades? S.fillimp/S.trades:0;
        double wr=S.trades? 100.0*S.wins/S.trades:0;
        printf("%-18s %7d %7d %8.2f %+8.1f%% %+8.2f%% %+7.2f%% %7.1f%% %8.2f  %+.1f/%+.1f%%\n",
               variants[vi].name, S.signals, S.trades, dep_avg, alloc*100, netdep*100,
               fillimp*100, wr, sharpe(S.tr), sum(h1)*100, sum(h2)*100);
        if(vi==0) base=S;
    }
    printf("\nNET/ALLOC = total return per 1.0 unit of the (identical) per-signal capital budget — THE fair capital-matched metric.\n");
    printf("NET/DEP   = return per unit actually deployed (entry/edge quality). FILLIMP = avg (open-blendedEntry)/open (>0 = cheaper fill).\n");
    printf("DEP.AVG   = avg fraction of the budget deployed (tranches that never fill = undeployed cash).\n");
    printf("\n================= DONE =================\n");
    return 0;
}

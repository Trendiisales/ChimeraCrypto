// S55: GRID / passive market-making sim — the maker-native edge for spot-long.
// Post limit buys below + limit sells above; each g% oscillation = a maker round
// trip earning g% - fee. Long-only: hold USDT, buy dips, sell rips. Risk = trend
// (esp down) leaves inventory underwater. Tests grid vs BUY-HOLD across regimes,
// at MAKER cost (~5bp round-trip), with an inventory cap + optional macro filter.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <glob.h>
#include <algorithm>
#include <cmath>

struct K { long long ts; double o,h,l,c; };
static void parse_file(const char* path, std::vector<K>& out){
    FILE* f=fopen(path,"rb"); if(!f) return;
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    std::string s; s.resize(n); size_t rd=fread(&s[0],1,n,f); fclose(f); s.resize(rd);
    const char* p=s.c_str(); const char* e=p+s.size();
    while(p<e){ while(p<e&&*p!='[')p++; if(p>=e)break; p++;
        while(p<e&&(*p==' '||*p=='['))p++;
        K k{}; k.ts=strtoll(p,(char**)&p,10);
        auto qd=[&](double&v){while(p<e&&*p!='"')p++; if(p<e){p++; v=strtod(p,(char**)&p); while(p<e&&*p!='"')p++; if(p<e)p++;}};
        qd(k.o);qd(k.h);qd(k.l);qd(k.c);
        if(k.ts>0&&k.c>0) out.push_back(k);
        while(p<e&&*p!=']')p++; if(p<e)p++;
    }
}

// Grid over [lo,hi] of bars. g=spacing frac, maxlots=inventory cap, fee=round-trip frac.
// macro_filter: only BUY while price>SMA(macroW) (don't catch a falling knife).
// Returns: total return frac (realized + open marked-to-last), and fills count.
struct Res{ double ret; int fills; double maxdd; };
static Res grid(std::vector<K>& b,int lo,int hi,double g,int maxlots,double fee,bool macro,int macroW){
    double cash=1.0;                 // start with 1.0 USDT of "dry powder" per lot slot
    double per_lot = 1.0/maxlots;    // size each buy
    std::vector<double> lots;        // entry prices of open long lots
    double last_buy = b[lo].c;
    int fills=0;
    double peak_eq=1.0, maxdd=0.0;
    for(int i=lo;i<hi;i++){
        double px=b[i].c;
        // macro filter: SMA over macroW bars
        bool can_buy=true;
        if(macro && i>=macroW){ double s=0; for(int j=i-macroW;j<i;j++) s+=b[j].c; can_buy = px > s/macroW; }
        // BUY a lot if price dropped g below last buy and slot free and (macro ok)
        if(can_buy && (int)lots.size()<maxlots && px <= last_buy*(1-g)){
            lots.push_back(px); last_buy=px; cash-=per_lot; fills++;
        }
        // SELL any lot that's risen g above its entry (maker), FIFO-ish (lowest first)
        std::sort(lots.begin(),lots.end());
        for(size_t k=0;k<lots.size();){
            if(px >= lots[k]*(1+g)){
                double profit = per_lot * (px/lots[k]-1.0 - fee);  // realized
                cash += per_lot + profit;
                last_buy = px;          // reset reference after a sell
                lots.erase(lots.begin()+k); fills++;
            } else k++;
        }
        // equity = cash + open lots marked to market
        double eq=cash; for(double lp:lots) eq += per_lot*(px/lp);
        if(eq>peak_eq)peak_eq=eq; double dd=peak_eq-eq; if(dd>maxdd)maxdd=dd;
    }
    // close: mark open lots to last price
    double px=b[hi-1].c, eq=cash; for(double lp:lots) eq += per_lot*(px/lp - fee);
    return {eq-1.0, fills, maxdd};
}

int main(int argc,char**argv){
    const char* sym = argc>1?argv[1]:"btc";
    std::vector<K> bars;
    std::string pat=std::string("data/")+sym+"_h1_part*.json";
    glob_t G; glob(pat.c_str(),0,nullptr,&G);
    std::vector<std::string> fs(G.gl_pathv,G.gl_pathv+G.gl_pathc); globfree(&G);
    std::sort(fs.begin(),fs.end(),[](const std::string&a,const std::string&b){
        auto nm=[](const std::string&s){size_t p=s.find("part");return atoi(s.c_str()+p+4);};return nm(a)<nm(b);});
    for(auto&f:fs) parse_file(f.c_str(),bars);
    std::sort(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts<b.ts;});
    printf("GRID sim — %s, %zu H1 bars, maker fee 5bp round-trip\n\n",sym,bars.size());

    int W=2160; double fee=0.0005;
    // sweep spacing g x inventory cap; report avg return per 90d + how often >0.
    printf("spacing  maxlots | avg_ret/90d  pos_slices  ann%%\n");
    double bhsum=0; int N=0; for(int s=200;s+W<(int)bars.size();s+=W){bhsum+=bars[s+W-1].c/bars[s].c-1.0;N++;}
    printf("(buy-hold avg %+.1f%%/90d for reference)\n",100*bhsum/N);
    for(double g : {0.005,0.01,0.015,0.02,0.03}){
        for(int ml : {10,20,40}){
            double sum=0; int n=0,pos=0;
            for(int s=200; s+W<(int)bars.size(); s+=W){
                Res r=grid(bars,s,s+W,g,ml,fee,false,0);
                sum+=r.ret; n++; if(r.ret>0)pos++;
            }
            double avg=sum/n;
            printf("%-8.3f %-7d | %+-11.1f %d/%d       %+.0f%%\n",g,ml,100*avg,pos,n,100*(pow(1+avg,365.0/90)-1));
        }
    }
    return 0;
}

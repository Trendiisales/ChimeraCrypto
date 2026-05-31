// Validate GridEngine matches the sim (~+10%/yr on BTC) on real data.
#include "core/GridEngine.hpp"
#include <cstdio>
#include <cstdlib>
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
int main(int argc,char**argv){
    const char* sym=argc>1?argv[1]:"btc";
    std::vector<K> bars; std::string pat=std::string("data/")+sym+"_h1_part*.json";
    glob_t G; glob(pat.c_str(),0,nullptr,&G);
    std::vector<std::string> fs(G.gl_pathv,G.gl_pathv+G.gl_pathc); globfree(&G);
    std::sort(fs.begin(),fs.end(),[](const std::string&a,const std::string&b){auto nm=[](const std::string&s){size_t p=s.find("part");return atoi(s.c_str()+p+4);};return nm(a)<nm(b);});
    for(auto&f:fs) parse_file(f.c_str(),bars);
    std::sort(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts<b.ts;});

    // CONTINUOUS run over full history, carrying inventory, with macro-200d gate
    // (buy only when price > 200d-SMA). Honest: inventory bought pre-bear is held.
    int MA=200*24;
    chimera::GridEngine::Config c; c.symbol=sym; c.tag="GRID"; c.grid_pct=0.02; c.max_lots=12;
    chimera::GridEngine g(c);
    double peak=1.0, maxdd=0.0;
    for(int i=MA;i<(int)bars.size();i++){
        double sma=0; for(int j=i-MA;j<i;j++) sma+=bars[j].c; sma/=MA;
        bool macro = bars[i].c > sma;
        g.on_tick(bars[i].c, bars[i].ts, macro);
        double eq=g.equity_(bars[i].c);
        if(eq>peak)peak=eq; double dd=(peak-eq)/peak; if(dd>maxdd)maxdd=dd;
    }
    double yrs_f=(bars.back().ts-bars[MA].ts)/1000.0/86400/365.0;
    double final_eq=g.equity_(bars.back().c);
    double ann=pow(final_eq, 1.0/yrs_f)-1.0;
    printf("GridEngine %-8s CONTINUOUS %.1fyr (macro-200d gated): final_eq=%.2fx  ann=%+.0f%%  maxDD=%.0f%%  fills=%d  open_lots=%d\n",
           sym, yrs_f, final_eq, 100*ann, 100*maxdd, g.fills(), g.open_lots());
    return final_eq>1.0?0:1;
}

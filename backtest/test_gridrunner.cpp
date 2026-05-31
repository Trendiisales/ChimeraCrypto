// S55: GRID+RUNNER hybrid vs plain GRID. Plain grid sells every lot at +g%. Hybrid:
// the FIRST lot to reach +g% becomes a RUNNER (don't sell — trail it with a wide
// stop, riding the sustained move until a pullback), while other lots keep locking
// +g%. Question: does the runner lot ADD net return over plain grid, or just add
// whipsaw? Real data, 5bp maker, macro-gate via caller. Reports both.
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

// runner_trail=0 -> plain grid (sell all at +g). >0 -> hybrid (first +g lot runs, trails).
static double run(std::vector<K>&b,int lo,int hi,double g,int maxlots,double fee,double runner_trail){
    double per=1.0/maxlots; std::vector<double> lots; double ref=b[lo].c, realized=0;
    bool has_runner=false; double run_entry=0, run_peak=0;
    for(int i=lo;i<hi;i++){
        double px=b[i].c;
        // runner trail
        if(has_runner){
            if(px>run_peak) run_peak=px;
            if(px <= run_peak*(1-runner_trail)){
                realized += per*(px/run_entry-1.0-fee); has_runner=false;
            }
        }
        // buy on g-drop
        if((int)lots.size()+(has_runner?1:0) < maxlots && px <= ref*(1-g)){ lots.push_back(px); ref=px; }
        // sell lots at +g (first one becomes runner if hybrid + no runner)
        std::sort(lots.begin(),lots.end());
        for(size_t k=0;k<lots.size();){
            if(px >= lots[k]*(1+g)){
                if(runner_trail>0 && !has_runner){ has_runner=true; run_entry=lots[k]; run_peak=px; }
                else { realized += per*(px/lots[k]-1.0-fee); }
                ref=px; lots.erase(lots.begin()+k);
            } else k++;
        }
    }
    double px=b[hi-1].c, eq=1.0+realized;
    for(double lp:lots) eq += per*(px/lp - fee);
    if(has_runner) eq += per*(px/run_entry - fee);
    return eq-1.0;
}

int main(int argc,char**argv){
    const char* sym=argc>1?argv[1]:"btc";
    std::vector<K> bars; std::string pat=std::string("data/")+sym+"_h1_part*.json";
    glob_t G; glob(pat.c_str(),0,nullptr,&G);
    std::vector<std::string> fs(G.gl_pathv,G.gl_pathv+G.gl_pathc); globfree(&G);
    std::sort(fs.begin(),fs.end(),[](const std::string&a,const std::string&b){auto nm=[](const std::string&s){size_t p=s.find("part");return atoi(s.c_str()+p+4);};return nm(a)<nm(b);});
    for(auto&f:fs) parse_file(f.c_str(),bars);
    std::sort(bars.begin(),bars.end(),[](const K&a,const K&b){return a.ts<b.ts;});

    int W=2160; double fee=0.0005,g=0.02;
    double sg=0,sr=0; int n=0,rwin=0;
    for(int s=200;s+W<(int)bars.size();s+=W){
        double grid=run(bars,s,s+W,g,12,fee,0.0);
        double hyb =run(bars,s,s+W,g,12,fee,0.05);   // runner trails 5%
        sg+=grid; sr+=hyb; n++; if(hyb>grid)rwin++;
    }
    printf("%-9s | plain grid avg %+.1f%%/90d | grid+runner avg %+.1f%%/90d | runner better in %d/%d slices\n",
           sym,100*sg/n,100*sr/n,rwin,n);
    return 0;
}

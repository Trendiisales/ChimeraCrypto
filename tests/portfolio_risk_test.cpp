// portfolio_risk_test.cpp — Phase-3 item 17.
// A highly-correlated cluster is NOT diversification: its contribution to
// portfolio variance is capped from ACTUAL rolling returns (shrunk covariance),
// so the cluster's weight is scaled down. Also proves vol-target and crypto-beta
// scalars behave.
#include "live/PortfolioRisk.hpp"
#include <cstdio>
#include <cmath>
#include <map>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

int main() {
    PortfolioRisk R; R.configure(/*window*/40, /*shrink*/0.20);

    // Cluster 0: AAA + BBB move IDENTICALLY (corr ~ +1) — a redundant cluster.
    // Cluster 1: CCC moves independently.
    for (int i = 0; i < 40; ++i) {
        double a = std::sin(i * 0.5) * 0.03;           // AAA
        double c = std::cos(i * 0.37) * 0.03;          // CCC, decorrelated
        R.observe("AAA", a);
        R.observe("BBB", a);                            // == AAA -> perfectly correlated
        R.observe("CCC", c);
        R.observe("BTCUSDT", a * 0.5 + c * 0.5);        // benchmark
    }
    CHECK(R.correlation("AAA","BBB") > 0.99);
    CHECK(std::fabs(R.correlation("AAA","CCC")) < 0.6);

    std::map<std::string,double> w = { {"AAA",1000.0}, {"BBB",1000.0}, {"CCC",1000.0} };
    std::map<std::string,int> cl = { {"AAA",0}, {"BBB",0}, {"CCC",1} };

    // pre-cap: cluster 0 (AAA+BBB, perfectly correlated) dominates variance.
    double V = R.portfolio_variance(w);
    auto contrib = [&](std::map<std::string,double> ww, int cid){
        double av = 0; int n=0; for (auto&kv:ww){av+=R.variance(kv.first);++n;} av/= (n?n:1);
        double s=0; for (auto& i:ww){ if (cl[i.first]!=cid) continue;
            double sw=0; for (auto& j:ww) sw += j.second * R.shrunk_cov(i.first,j.first,av);
            s += i.second*sw; } return s; };
    double c0_before = contrib(w,0)/V;
    std::printf("[info] cluster0 variance share before cap = %.3f\n", c0_before);
    CHECK(c0_before > 0.5);                              // redundant cluster over half the risk

    // cap any cluster to 50% of portfolio variance.
    auto capped = R.cap_clusters(w, cl, /*max_frac*/0.50);
    double Vc = R.portfolio_variance(capped);
    double c0_after = contrib(capped,0)/Vc;
    double w0_before = w["AAA"]+w["BBB"], w0_after = capped["AAA"]+capped["BBB"];
    std::printf("[info] cluster0 share after cap = %.3f | cluster0 weight %.0f -> %.0f\n",
                c0_after, w0_before, w0_after);
    CHECK(w0_after < w0_before);                         // the correlated cluster scaled down
    CHECK(c0_after <= 0.50 + 0.03);                      // contribution now within the cap

    // vol-target scalar never levers up, and scales DOWN a hot book.
    double s_hi = R.vol_target_scale(w, /*target*/0.001);  // tiny target -> scale down
    double s_lo = R.vol_target_scale(w, /*target*/1e9);    // huge target -> clamp at 1.0
    std::printf("[info] vol_target_scale small=%.4f large=%.4f\n", s_hi, s_lo);
    CHECK(s_hi < 1.0);
    CHECK(std::fabs(s_lo - 1.0) < 1e-9);

    // crypto-beta scale caps a high-beta book.
    double bscale = R.crypto_beta_scale(w, "BTCUSDT", /*max_beta*/0.1);
    std::printf("[info] portfolio beta=%.3f -> beta_scale=%.4f\n",
                R.portfolio_beta(w,"BTCUSDT"), bscale);
    CHECK(bscale <= 1.0);

    std::printf(fails==0 ? "PASS: covariance cluster cap + vol-target + beta cap\n"
                         : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}

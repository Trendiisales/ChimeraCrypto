#pragma once
#include <cstdlib>
// ============================================================================
// FundingRateFetcher — BTC perpetual funding rate from Binance FAPI
// ============================================================================
// Fetched on startup and every 8h via curl shell command (no libcurl linking).
//
// INTERPRETATION:
//   > +0.0002 (20bp/8h): longs crowded -> reduce long size 25%
//   > +0.0005 (50bp/8h): heavily crowded -> reduce long size 50%
//   < -0.0003: shorts crowded -> longs have carry edge, size up 20%
//   ~0: neutral -> no adjustment
// ============================================================================
#include <cstdio>
#include <atomic>

namespace chimera {

class FundingRateFetcher {
public:
    void fetch() {
        int ret = ::system(
            "curl -s 'https://fapi.binance.com/fapi/v1/premiumIndex?symbol=BTCUSDT'"
            " | grep -o '\"lastFundingRate\":\"[^\"]*\"'"
            " | grep -o '[0-9.-]*' > /tmp/chimera_funding.txt 2>/dev/null"
        );
        (void)ret;

        FILE* f = std::fopen("/tmp/chimera_funding.txt", "r");
        if (f) {
            double val = 0.0;
            if (std::fscanf(f, "%lf", &val) == 1) {
                rate_.store(val);
                fetched_.store(true);
                std::printf("[FUNDING] BTC funding rate: %.5f%% (%.2fbp/8h) | long_mult=%.2f\n",
                    val * 100.0, val * 10000.0, long_size_multiplier_for(val));
                std::fflush(stdout);
            }
            std::fclose(f);
        }
    }

    double rate() const { return rate_.load(); }
    bool   ready() const { return fetched_.load(); }

    double long_size_multiplier() const {
        return long_size_multiplier_for(rate_.load());
    }

private:
    static double long_size_multiplier_for(double r) {
        if (r >  0.0005) return 0.5;   // >50bp/8h — very crowded longs
        if (r >  0.0002) return 0.75;  // >20bp/8h — crowded
        if (r < -0.0003) return 1.2;   // negative — longs have carry edge
        return 1.0;
    }

    std::atomic<double> rate_{0.0};
    std::atomic<bool>   fetched_{false};
};

} // namespace chimera

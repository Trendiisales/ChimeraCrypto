#pragma once
#include <algorithm>
#include <cmath>

namespace chimera {

class VolatilityScoring {
public:
    VolatilityScoring() : range_10s_high_(0), range_10s_low_(0), 
                          range_60s_high_(0), range_60s_low_(0) {}
    
    void update(double price, int64_t now_ms) {
        // Track 10s range
        if (now_ms - last_10s_reset_ >= 10000) {
            range_10s_high_ = price;
            range_10s_low_ = price;
            last_10s_reset_ = now_ms;
        } else {
            range_10s_high_ = std::max(range_10s_high_, price);
            range_10s_low_ = std::min(range_10s_low_, price);
        }
        
        // Track 60s range
        if (now_ms - last_60s_reset_ >= 60000) {
            range_60s_high_ = price;
            range_60s_low_ = price;
            last_60s_reset_ = now_ms;
        } else {
            range_60s_high_ = std::max(range_60s_high_, price);
            range_60s_low_ = std::min(range_60s_low_, price);
        }
    }
    
    double get_vol_score() const {
        double short_range = range_10s_high_ - range_10s_low_;
        double long_range = range_60s_high_ - range_60s_low_;
        
        if (long_range == 0.0) return 1.0;
        return short_range / long_range;
    }
    
    double get_size_multiplier(double vol_score, double latency_p95) const {
        double mult;
        
        if (vol_score < 0.7) mult = 0.6;
        else if (vol_score <= 1.3) mult = 1.0;
        else if (vol_score <= 1.8) mult = 1.8;
        else mult = 2.3;
        
        // Clamp on latency spike
        if (latency_p95 > 12.0) {
            mult = std::min(mult, 0.7);
        }
        
        return mult;
    }

private:
    double range_10s_high_;
    double range_10s_low_;
    double range_60s_high_;
    double range_60s_low_;
    int64_t last_10s_reset_ = 0;
    int64_t last_60s_reset_ = 0;
};

}

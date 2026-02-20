#pragma once
#include "alpha/AlphaBase.hpp"
#include "alpha/ShockAlpha.hpp"
#include "alpha/ImbalanceVelocity.hpp"
#include "alpha/LiquidityVacuum.hpp"
#include "alpha/SpreadCompression.hpp"

namespace chimera {

template<int N>
struct AlphaSet {
    struct Signal {
        double value = 0.0;
        double weight = 1.0;
        uint64_t last_update = 0;
        double decay_rate = 0.1;
    };
    
    Signal signals[N];
    
    double fused_signal(uint64_t now_us) const {
        double total_weight = 0.0;
        double weighted_sum = 0.0;
        
        for (int i = 0; i < N; ++i) {
            double dt = (now_us - signals[i].last_update) / 1000000.0;
            double decayed = signals[i].value * __builtin_exp(-signals[i].decay_rate * dt);
            weighted_sum += decayed * signals[i].weight;
            total_weight += signals[i].weight;
        }
        
        return (total_weight > 0.0) ? weighted_sum / total_weight : 0.0;
    }
};

}

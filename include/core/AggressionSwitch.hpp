#pragma once

namespace chimera {

class AggressionSwitch {
public:
    bool should_cross(double survival_prob, double latency_ms, double spread_bps) {
        if(survival_prob < 0.25 && latency_ms < 5.0 && spread_bps < 8.0)
            return true;
        return false;
    }
};

}

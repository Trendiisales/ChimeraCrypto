#pragma once

namespace chimera {

class AdaptiveThrottle {
public:
    void record_latency(double ms);
    bool allow() const;

private:
    double avg_latency_ = 0;
};

}

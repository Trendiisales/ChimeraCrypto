#pragma once
#include <cmath>

namespace chimera {

class LiquiditySweepDetector {
public:
    void update(double prevBidVol, double newBidVol,
               double prevAskVol, double newAskVol) {
        double bidDrop = prevBidVol - newBidVol;
        double askDrop = prevAskVol - newAskVol;
        
        sweepScore_ = 0.9 * sweepScore_ + 
                     0.1 * (std::fabs(bidDrop) + std::fabs(askDrop));
    }

    bool sweep() const {
        return sweepScore_ > threshold_;
    }

    double score() const {
        return sweepScore_;
    }

private:
    double sweepScore_ = 0.0;
    double threshold_ = 5.0;
};

} // namespace chimera

#pragma once
#include "MicroburstDetector.hpp"
#include "OrderFlowAcceleration.hpp"
#include "LiquiditySweepDetector.hpp"
#include "ReversionDecayModel.hpp"
#include "LeadLagAlpha.hpp"

namespace chimera {

class EdgeEngine {
public:
    void update(double imbalance, double tradeVol, double priceReturn,
               double leaderRet, double followerRet,
               double prevBidVol, double newBidVol,
               double prevAskVol, double newAskVol) {
        microburst_.update(tradeVol);
        accel_.update(imbalance);
        sweep_.update(prevBidVol, newBidVol, prevAskVol, newAskVol);
        reversion_.update(priceReturn);
        leadLag_.update(leaderRet, followerRet);
    }

    double expectedEdgeBps() const {
        double edge = 0.0;

        edge += accel_.acceleration() * 20.0;
        edge += microburst_.score() * 5.0;
        edge += sweep_.score() * 3.0;
        edge += leadLag_.edge() * 15.0;

        edge *= reversion_.multiplier();

        return edge;
    }

private:
    MicroburstDetector microburst_;
    OrderFlowAcceleration accel_;
    LiquiditySweepDetector sweep_;
    ReversionDecayModel reversion_;
    LeadLagAlpha leadLag_;
};

} // namespace chimera

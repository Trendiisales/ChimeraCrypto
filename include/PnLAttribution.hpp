#pragma once

namespace chimera {

struct PnLBreakdown {
    double grossBps = 0.0;
    double spreadCostBps = 0.0;
    double feeCostBps = 0.0;
    double slippageBps = 0.0;
    double netBps = 0.0;
};

class PnLAttribution {
public:
    PnLBreakdown compute(double entry, double exit, double spreadBps,
                        double feeBps, double slippageBps, bool longSide) {
        double raw = (exit - entry) / entry;
        if (!longSide) {
            raw = -raw;
        }

        double gross = raw * 10000.0;
        double net = gross - spreadBps - feeBps - slippageBps;

        return { gross, spreadBps, feeBps, slippageBps, net };
    }
};

} // namespace chimera

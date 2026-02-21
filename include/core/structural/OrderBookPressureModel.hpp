#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace chimera {

/**
 * OrderBookPressureModel - L2 Imbalance + Aggression Detector
 * 
 * Detects:
 * - Bid vs ask volume imbalance
 * - Aggressive market order flow
 * - Liquidity pull (passive side withdrawing)
 * - Microstructure exhaustion
 * 
 * Outputs directional pressure score for entry timing.
 */
class OrderBookPressureModel {
public:

    struct BookSnapshot {
        double bid_vol;
        double ask_vol;
        double bid_delta;
        double ask_delta;
        double trade_aggression; // positive = buy aggression, negative = sell
        double mid_price;
        double spread;
        int64_t ts;
    };

    struct Output {
        double imbalance_score;
        double aggression_score;
        double exhaustion_score;
        double composite_score;
        int direction;  // -1 sell, 0 neutral, 1 buy
    };

private:

    double imbalance_ema_ = 0.0;
    double aggression_ema_ = 0.0;
    double exhaustion_ema_ = 0.0;

    double prev_bid_vol_ = 0.0;
    double prev_ask_vol_ = 0.0;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

public:

    Output update(const BookSnapshot& s) {

        double total_vol = s.bid_vol + s.ask_vol + 1e-9;
        double imbalance = (s.bid_vol - s.ask_vol) / total_vol;

        imbalance_ema_ = ema(imbalance_ema_, imbalance, 0.1);

        double aggression = s.trade_aggression;
        aggression_ema_ = ema(aggression_ema_, aggression, 0.15);

        double pull_bid = prev_bid_vol_ - s.bid_vol;
        double pull_ask = prev_ask_vol_ - s.ask_vol;

        double exhaustion = 0.0;

        if (aggression_ema_ > 0 && pull_ask > 0)
            exhaustion = -std::abs(pull_ask);

        if (aggression_ema_ < 0 && pull_bid > 0)
            exhaustion = std::abs(pull_bid);

        exhaustion_ema_ = ema(exhaustion_ema_, exhaustion, 0.2);

        prev_bid_vol_ = s.bid_vol;
        prev_ask_vol_ = s.ask_vol;

        double composite =
            imbalance_ema_ * 0.4
          + aggression_ema_ * 0.4
          + exhaustion_ema_ * 0.2;

        composite = clamp(composite, -5.0, 5.0);

        int direction = 0;
        if (composite > 0.5) direction = 1;
        if (composite < -0.5) direction = -1;

        Output o;
        o.imbalance_score = imbalance_ema_;
        o.aggression_score = aggression_ema_;
        o.exhaustion_score = exhaustion_ema_;
        o.composite_score = composite;
        o.direction = direction;

        return o;
    }
};

} // namespace chimera

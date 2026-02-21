#pragma once
#include <cmath>
#include <cstdint>

namespace chimera {

struct MicroSnapshot {
    double bid = 0.0;
    double ask = 0.0;
    double mid = 0.0;

    double short_range = 0.0;
    double long_range = 0.0;

    double last_price = 0.0;
    double prev_price = 0.0;

    double spread_bps = 0.0;
    double latency_ms = 0.0;
};

struct MicroConfig {
    double min_rel_strength = 2.2;
    double min_displacement_bps = 4.0;
    double continuation_factor = 1.4;
    double cost_floor_bps = 4.0;
};

class MicroSignalEngine {
public:
    explicit MicroSignalEngine(const MicroConfig& cfg)
        : cfg_(cfg) {}

    bool generate_signal(const MicroSnapshot& s,
                         double& expected_bps,
                         double& rel_strength,
                         int& direction)
    {
        rel_strength = compute_rel(s);
        if (rel_strength < cfg_.min_rel_strength)
            return false;

        double displacement = compute_displacement_bps(s);
        if (std::fabs(displacement) < cfg_.min_displacement_bps)
            return false;

        if (!continuation_confirmed(s, displacement))
            return false;

        double projected = project_move(rel_strength, displacement);

        if (projected < cfg_.cost_floor_bps + s.spread_bps)
            return false;

        expected_bps = projected;
        direction = displacement > 0 ? 1 : -1;

        return true;
    }

private:
    MicroConfig cfg_;

    double compute_rel(const MicroSnapshot& s) const
    {
        if (s.long_range <= 0.0) return 1.0;
        return s.short_range / s.long_range;
    }

    double compute_displacement_bps(const MicroSnapshot& s) const
    {
        if (s.prev_price <= 0.0) return 0.0;
        double move = (s.last_price - s.prev_price) / s.prev_price;
        return move * 10000.0;
    }

    bool continuation_confirmed(const MicroSnapshot& s,
                                double displacement) const
    {
        double second_leg = (s.mid - s.prev_price) / s.prev_price * 10000.0;

        if (displacement > 0)
            return second_leg > displacement / cfg_.continuation_factor;

        return second_leg < displacement / cfg_.continuation_factor;
    }

    double project_move(double rel, double displacement) const
    {
        double strength_boost = (rel - 2.0) * 3.0;
        return std::fabs(displacement) + strength_boost;
    }
};

}

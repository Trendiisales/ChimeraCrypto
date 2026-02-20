#pragma once
#include <cmath>

namespace chimera {

struct LatencyPolicyResult {
    bool allow;
    double size_multiplier;
};

class LatencyTracker {
public:
    LatencyTracker() : ema_latency_(5.0) {}
    
    void update(double latency_ms) {
        double alpha = 0.02;
        ema_latency_ = alpha * latency_ms + (1.0 - alpha) * ema_latency_;
    }

    double dynamicHardLimit() const {
        return ema_latency_ * 4.0;
    }

    double dynamicMediumLimit() const {
        return ema_latency_ * 2.5;
    }

    double dynamicSoftLimit() const {
        return ema_latency_ * 1.8;
    }
    
    double baseline() const {
        return ema_latency_;
    }

private:
    double ema_latency_;
};

class TieredLatencyGovernor {
public:
    TieredLatencyGovernor() : strong_threshold_(1.4) {}
    
    LatencyPolicyResult evaluate(double latency_ms, double impulse_strength) {
        LatencyPolicyResult r;

        double hard_limit = tracker_.dynamicHardLimit();
        double medium_limit = tracker_.dynamicMediumLimit();
        double soft_limit = tracker_.dynamicSoftLimit();

        if (latency_ms > hard_limit) {
            r.allow = false;
            r.size_multiplier = 0.0;
            return r;
        }

        if (latency_ms > medium_limit) {
            r.allow = true;
            r.size_multiplier = 0.5;
            return r;
        }

        if (latency_ms > soft_limit) {
            if (impulse_strength < strong_threshold_) {
                r.allow = false;
                r.size_multiplier = 0.0;
                return r;
            }
            r.allow = true;
            r.size_multiplier = 0.75;
            return r;
        }

        r.allow = true;
        r.size_multiplier = 1.0;
        return r;
    }
    
    void update(double latency_ms) {
        tracker_.update(latency_ms);
    }

    void setStrongThreshold(double v) {
        strong_threshold_ = v;
    }

private:
    LatencyTracker tracker_;
    double strong_threshold_;
};

struct SymbolPerformance {
    double realized_pnl = 0.0;
    int trades = 0;

    double averagePnL() const {
        if (trades == 0) return 0.0;
        return realized_pnl / trades;
    }

    bool viable() const {
        if (trades < 10) return true;
        return averagePnL() > 0.0;
    }
};

}

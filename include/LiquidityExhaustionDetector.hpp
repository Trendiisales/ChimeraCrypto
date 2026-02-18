#pragma once
#include <deque>
#include <cmath>

namespace chimera {

class LiquidityExhaustionDetector {
public:
    void update(double bidVol, double askVol) {
        double imb = (bidVol - askVol) / (bidVol + askVol + 1e-9);
        history_.push_back(imb);
        if (history_.size() > 20) {
            history_.pop_front();
        }
    }

    bool exhausted() const {
        if (history_.size() < 10) {
            return false;
        }

        int count = 0;
        for (double v : history_) {
            if (std::fabs(v) > 0.8) {
                count++;
            }
        }

        return count > 7;
    }

private:
    std::deque<double> history_;
};

} // namespace chimera

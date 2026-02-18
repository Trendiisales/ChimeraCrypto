#pragma once
#include <unordered_map>
#include <cmath>
#include <string>

namespace chimera {

class PortfolioSkewController {
public:
    void updateExposure(const std::string& symbol, double exposure) {
        exposures_[symbol] = exposure;
    }

    double scale(const std::string& symbol, double size) {
        double total = 0.0;
        for (const auto& p : exposures_) {
            total += std::fabs(p.second);
        }

        if (total > maxExposure_) {
            return size * 0.5;
        }

        return size;
    }

private:
    std::unordered_map<std::string, double> exposures_;
    double maxExposure_ = 100000.0;
};

} // namespace chimera

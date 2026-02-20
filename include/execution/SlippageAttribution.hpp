#pragma once
#include <unordered_map>
#include <string>

namespace chimera {

struct SlippageMetrics {
    double expected_price = 0.0;
    double fill_price = 0.0;
    double slippage_bps = 0.0;
};

class SlippageAttribution {
public:
    void record_expected(const std::string& clordid,
                         double price);

    SlippageMetrics on_fill(const std::string& clordid,
                            double fill_price,
                            bool is_buy);

private:
    std::unordered_map<std::string, double> expected_;
};

}

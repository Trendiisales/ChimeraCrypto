#include "execution/SlippageAttribution.hpp"

namespace chimera {

void SlippageAttribution::record_expected(
    const std::string& clordid,
    double price)
{
    expected_[clordid] = price;
}

SlippageMetrics SlippageAttribution::on_fill(
    const std::string& clordid,
    double fill_price,
    bool is_buy)
{
    SlippageMetrics m;

    auto it = expected_.find(clordid);
    if (it == expected_.end())
        return m;

    m.expected_price = it->second;
    m.fill_price = fill_price;

    double diff =
        fill_price - m.expected_price;

    if (!is_buy)
        diff = -diff;

    m.slippage_bps =
        (diff / m.expected_price) * 10000.0;

    expected_.erase(it);

    return m;
}

}

#include "alpha/ShockAlpha.hpp"

namespace chimera {

AlphaSignal ShockAlpha::evaluate(const std::string& symbol,
                                  double bid,
                                  double ask)
{
    AlphaSignal s;
    s.symbol = symbol;
    s.score = ask - bid;
    s.confidence = 0.5;
    s.size = 1.0;
    return s;
}

}

#pragma once
#include "types/AlphaSignal.hpp"

namespace chimera {

class ShockAlpha {
public:
    AlphaSignal evaluate(const std::string& symbol,
                         double bid,
                         double ask);
};

}

#pragma once
#include "types/AlphaSignal.hpp"

namespace chimera {

class ShockScorer {
public:
    AlphaSignal score(const AlphaSignal& in);
};

}
